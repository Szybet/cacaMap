/*
Copyright 2010 Jean Fairlie jmfairlie@gmail.com

This file is part of CacaMap
CacaMap is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "cacamap.h"
#include <QtWidgets>

using namespace std;
/**
* constructor
*/
longPoint::longPoint(quint32 _x, quint32 _y)
{
	x = _x;
	y = _y;
}
/**
*empty constructor
*/

longPoint::longPoint()
{
	x = 0;
	y = 0;
}

/**
* Converts a geo coordinate to map pixels
* @param geocoord has the longitude and latitude in degrees.
* @param zoom is the zoom level, ranges from cacaMap::minZoom (out) to maxZoom (in).
* @param tilesize the width/height in px of the square %tile (e.g 256).
* @return a longpoint struct containing the x and y px coordinates
* in the map for the given geocoordinates and zoom level.
*/
longPoint myMercator::geoCoordToPixel(QPointF const &geocoord, int zoom, int tilesize)
{
 	qreal  longitude = geocoord.x();
	qreal  latitude = geocoord.y();
	
	//height, width of the whole map,this is, all tiles for a given zoom level put together
	quint32 mapsize =  (1<<zoom)*tilesize;
	
	qreal latitude_m = atanh(sin(latitude/180.0*M_PI))*180.0/M_PI;

	quint32 x = mapsize*(longitude + 180.0)/360.0;
	
	quint32 y = mapsize*(180.0 - latitude_m)/360.0;
	
	return longPoint(x,y);
}
/**
* Converts  map pixels to geo coordinates in degrees
* @param pixelcoord has the x and y px coordinates.
* @param zoom  is the zoom level, ranges from 0(out) to 18(in).
* @param tilesize the width/height in px of the square %tile (e.g 256).
* @return a QPointF object containing the latitude and longitude of 
* of the given location.
*/

QPointF myMercator::pixelToGeoCoord(longPoint const &pixelcoord, int zoom, int tilesize)
{
	long  x= pixelcoord.x;
	long  y= pixelcoord.y;
	
	//height, width of the whole map,this is, all tiles for a given zoom level put together
	quint32 mapsize =  (1<<zoom)*tilesize;

	
	qreal longitude = x*360.0/mapsize - 180.0;
	
	qreal latitude_m = 180.0 - y*360.0/mapsize;
	qreal latitude = asin(tanh(latitude_m*M_PI/180.0))*180/M_PI;
	
	return QPointF(longitude,latitude);
}
/**
* constructor
*/
cacaMap::cacaMap(QWidget* parent):QWidget(parent)
{
	if (!servermgr.loadConfigFile("tileservers.xml"))
	{
		std::cout<<"error loading server file."<<std::endl;
	}
	cacheSize = 0;
	maxZoom = 18;
    minZoom = 0;
    folder = QDir::currentPath();
    clearCache();
	loadCache();
    geocoords = QPointF(15.8564,61.4667);
	downloading = false;
	tileSize = 256;
    zoom = 3;
	manager = new QNetworkAccessManager(this);
    loadingAnim.setFileName("loading.png");
	loadingAnim.setScaledSize(QSize(tileSize,tileSize));
	loadingAnim.start();
	notAvailableTile.load("notavailable.jpeg");
	imgBuffer = new QPixmap(size());
	buffzoomrate = 1.0;
}

/**
Sets the latitude and longitude to the coords in newcoords
@param newcoords the new coordinates.
*/
void cacaMap::setGeoCoords(QPointF newcoords)
{
	//geocoords.setX(newcoords.x());
	//geocoords.setY(newcoords.y());
	geocoords = newcoords;
}
/**
* zooms in one level
* @return true if it zoomed succesfully, false otherwise (if maxZoom has been reached)
*/
bool cacaMap::zoomIn()
{
	if (zoom < maxZoom)
	{
		zoom++;
		downloadQueue.clear();
		updateContent();
        //qDebug() << "Zooming in";
		return true;
	}
	return false;
}
/**
* zooms out one level
* @return true if it zoomed succesfully, false otherwise (if minZoom has been reached)
*/
bool cacaMap::zoomOut()
{
	if (zoom > minZoom)
	{
		zoom--;
		downloadQueue.clear();
		updateContent();
        //qDebug() << "Zooming out";
		return true;
	}
	return false;
}

/**
* zooms to a specific level
* @return true if it is a valid level, false otherwise (if level is outside the valid range)
*/
bool cacaMap::setZoom(int level)
{
	if (level>= minZoom && level <= maxZoom)
	{
		zoom = level;
		downloadQueue.clear();
		updateContent();
		return true;
	}
	return false;
}

/**
* @return current geocoords
*/
QPointF cacaMap::getGeoCoords()
{
	return geocoords;
}
/**
*@return list of available tile server names
*/
QStringList cacaMap::getServerNames()
{
	return servermgr.getServerNames();		
}
/**
* Change tile server to the one in index
*/
void cacaMap::setServer(int index)
{
	servermgr.selectServer(index);
	downloadQueue.clear();
	loadCache();
	downloading = false;
	updateContent();
	update();
}
/**
*   @return current zoom level
*/
int cacaMap::getZoom()
{
	return zoom;
}	

/**
*@param zoom zoom level
*@param x tile x column
*@return string with the path to the folder containing the tiles in 
*for zoom level and x column
*/
QString cacaMap::getTilePath(int zoom,qint32 x)
{
	return "cache/"+servermgr.tileCacheFolder()+servermgr.filePath(zoom,x);
}

/**
* @return image for temporarily replacing a tile that is downloading and currently unavailable
* The 'patch' is a subsection of an available tile from a lower zoom level.
* The algorithm tries to find a suitable tile starting from level zoom -1, until level 0.
* The higher the zoom level difference the more pixelated the patch will be.
*/
QPixmap cacaMap::getTilePatch(int zoom, quint32 x, quint32 y, int offx, int offy, int tsize)
{
	//dont go beyond level 0 and 
	//dont use patches smaller than 16 px. They are unintelligible anyways.
	if (zoom>0 && tsize>=16*2)
	{
		int parentx, parenty, offsetx, offsety;
		QString tileid;
		QPixmap patch;
		QString sz,sx,sy;
		parentx = x/2;
		parenty = y/2;
		sz.setNum(zoom-1);
		sx.setNum(parentx);
		sy.setNum(parenty);
		offsetx = offx/2 + (x%2)*tileSize/2;
		offsety = offy/2 + (y%2)*tileSize/2;
		tileid = sz+"."+sx+"."+sy;
		if (tileCache.contains(tileid))
		{
			//render the tile
			QDir::setCurrent(folder);
			QString path= getTilePath(zoom-1,parentx) ;
			QString fileName = servermgr.fileName(parenty);
			QDir::setCurrent(path);
			QFile f(fileName);
			if (f.open(QIODevice::ReadOnly))
			{
				patch.loadFromData(f.readAll());
				f.close();
				return patch.copy(offsetx,offsety,tsize/2,tsize/2).scaledToHeight(tileSize);
			}
			else
			{
                qDebug() << "no file found "<< path << fileName;
			}
		}
		else
		{
			return getTilePatch(zoom-1,parentx,parenty,offsetx,offsety,tsize/2);
		}
	}
	return loadingAnim.currentPixmap();
}



/**
Starts downloading the next %tile in the queue
@see cacaMap::downloadQueue
*/
void cacaMap::downloadPicture()
{
	//check if there isnt an active download already
	if (!downloading)
	{
		//there are items in the queue
		if (downloadQueue.size())
		{
			downloading = true;
			QHash<QString,tile>::const_iterator i;
			i = downloadQueue.constBegin();
			tile nextItem = i.value();
			QString surl = nextItem.url;
			QNetworkRequest request;
			request.setUrl(QUrl(surl));
            // qDebug() << surl;
			QNetworkReply *reply = manager->get(request);
            //connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),this, SLOT(slotError(QNetworkReply::NetworkError)));
			connect(manager, SIGNAL(finished(QNetworkReply*)),this, SLOT(slotDownloadReady(QNetworkReply*)));
            //connect(reply, SIGNAL(downloadProgress(qint64,qint64)),this, SLOT(slotDownloadProgress(qint64, qint64)));
		}
		else
		{
		//	cout<<"no items in the queue"<<endl;
		}
	}
	else
	{
        //cout<<"another download is already in progress... "<<endl;
	}
}
/**
Populates the cache list by checking the existing files on the cache folder
*/
void cacaMap::loadCache()
{
	unavailableTiles.clear();
	tileCache.clear();
	QDir::setCurrent(folder);
	QDir dir;
	if (dir.cd("cache"))
	{
		if (dir.cd(servermgr.tileCacheFolder()))
		{
			QStringList zoom = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
			QString zoomLevel;
			for(int i=0; i< zoom.size(); i++)
			{
				zoomLevel = zoom.at(i);
				dir.cd(zoomLevel);
				QStringList longitudes = dir.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
				QString lon;
				for(int j=0; j< longitudes.size(); j++)
				{
					lon = longitudes.at(j);
					dir.cd(lon);
					QFileInfoList latitudes = dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
					QString lat;
					for(int k=0; k< latitudes.size(); k++)
					{
						lat = latitudes.at(k).baseName();
						QString name = zoomLevel+"."+lon+"."+lat;
						tileCache.insert(name,1);
					}
					dir.cdUp();//go back to zoom level folder
				}
				dir.cdUp();//go back to tile folder
			}
		}
		QDir::setCurrent(folder);
	}
}

/**
Slot to keep track of download progress
*/
void cacaMap::slotDownloadProgress(qint64 _bytesReceived, qint64 _bytesTotal)
{
}

/**
* Slot that gets called everytime a %tile download request finishes
* Saves image file to HDD, takes out item from download queue, and  adds item to cache list
*/
void cacaMap::slotDownloadReady(QNetworkReply * _reply)
{
	QNetworkReply::NetworkError error = _reply->error();
	//get url of original request 
	QNetworkRequest req = _reply->request();
	QUrl url = req.url();
	QString surl = url.toString();
	bool found = false;
	QHash<QString,tile>::const_iterator i;
	i = downloadQueue.constBegin();
	
	for( i; i!=downloadQueue.constEnd();i++)
	{
		if (i.value().url == surl)
		{
			found = true;
			break;
		}
	}

	if (error == QNetworkReply::NoError)
	{
		qint64 bytes = _reply->bytesAvailable();

		if (bytes)
		{
			//get url of original request 
			QNetworkRequest req = _reply->request();
			QUrl url = req.url();
			QString surl = url.toString();
			cacheSize+=bytes;

            // Here it looks if its too big
            // 30 * 1024 * 1024 = 31457280
            //qDebug() << "Cache size is: " << cacheSize;
            if(cacheSize > 31457280.0) {
                qDebug() << "Size is too big, clearing";
                clearCache();
            }

			//get image data
			QByteArray data = _reply->readAll();		
			if (found)
			{
				tile nextItem = i.value();
				QString kk = i.key();
				QString zdir = QString().setNum(nextItem.zoom);
				QString xdir = QString().setNum(nextItem.x);
				QString tilefile = servermgr.fileName(nextItem.y);

				QDir::setCurrent(folder);
				QDir dir;
				if (!dir.exists("cache"))
				{
                    qDebug() << "Creating dir cache";
					dir.mkdir("cache");
				}
				dir.cd("cache");
				if (!dir.exists(servermgr.tileCacheFolder()))
				{
					dir.mkdir(servermgr.tileCacheFolder());
				}
				dir.cd(servermgr.tileCacheFolder());

				if(!dir.exists(zdir))
				{
					dir.mkdir(zdir);	
				}
				dir.cd(zdir);
				if(!dir.exists(xdir))
				{
					dir.mkdir(xdir);	
				}
				dir.cd(xdir);
				
				QDir::setCurrent(dir.path());
				QFile f(tilefile);
				f.open(QIODevice::WriteOnly);
				quint64 byteswritten = f.write(data);
				if (byteswritten <= 0)
				{
					cout<<"error writing to file "<<f.fileName().toStdString()<<endl;
				}
				f.close();
				//remove item from download queue
				downloadQueue.remove(i.key());
				
				//add it to cache
				tileCache.insert(kk,1);
				//update with new tile
				updateBuffer();
				update();
			}
			else
			{
				cout<<"downloaded tile "<<surl.toStdString()<<" was not in Download queue. Data ignored"<<endl;
			}
			downloading = false;
			downloadPicture();
			
		}
		else
		{
			//cout<<"no data"<<endl;
		}
		update();
	}
	else
	{
		cout<<"network error: ("<<error<<") "<<_reply->errorString().toStdString()<<endl;
		
		if(found)
		{
			//if content is not available we dont want to keep requesting it
			if (error == QNetworkReply::ContentNotFoundError)
			{
				unavailableTiles.insert(i.key(),1);
			}
			//remove the item from queue and try again
			downloadQueue.remove(i.key());
			downloading = false;
			downloadPicture();
		}
	}
	_reply->deleteLater();
}
/**
Slot that gets called when theres is an network error
*/
void cacaMap::slotError(QNetworkReply::NetworkError _code)
{
    // cout<<"some error "<<_code<<endl;
}
/**
Widget resize event handler
*/
void cacaMap::resizeEvent(QResizeEvent* event)
{
	delete imgBuffer;
	imgBuffer = new QPixmap(size());
	updateContent();
}

/**
* Blits buffer to widget
*/
void cacaMap::renderMap(QPainter &p)
{
	//QRect dest(QPoint(0,0), size());
	if (buffzoomrate<1.0)
	{
		int ox = width()*(1-buffzoomrate)/2;
		int oy = height()*(1-buffzoomrate)/2;
		QRect src(QPoint(ox,oy),size()*buffzoomrate); 
		
		tmpbuff= imgBuffer->copy(src).scaled(size(),Qt::KeepAspectRatio);

		p.drawPixmap(0,0,tmpbuff);
		//p.drawPixmap(dest, *imgBuffer,src);//this is shorter but slower
	}
	else
	{
		p.drawPixmap(0,0,*imgBuffer);
	}
}
/**
Paint even handler
*/
void cacaMap::paintEvent(QPaintEvent *event)
{
	QPainter p(this);
	renderMap(p);
}

/**
destructor
*/
cacaMap::~cacaMap()
{
	delete manager;
	delete imgBuffer;
}
/**
Figures out which tiles are visible
Based on the current size of the widget, the %tile size, current coordinates and zoom level
*/
void cacaMap::updateTilesToRender()
{
	longPoint pixelCoords = myMercator::geoCoordToPixel(geocoords,zoom,tileSize); 

	//central tile coords
	qint32 xtile = pixelCoords.x/tileSize;
	qint32 ytile = pixelCoords.y/tileSize;
	//offset of central tile respect to the center of the widget
	int offsetx = pixelCoords.x % tileSize;
	int offsety = pixelCoords.y % tileSize;

	//num columns of tiles that fit left of the central tile
	float tilesleft = (float)(this->width()/2 - offsetx)/tileSize;
	
	//how many pixels overflow from the leftmost  tiles
	//second %tileSize is to take into account negative tilesLeft
	int globaloffsetx = (tileSize - (this->width()/2 - offsetx) % tileSize)%tileSize;

	//num rows of tiles that fit above the central tile
	float tilesup = (float)(this->height()/2 - offsety)/tileSize;

	//how many pixels overflow from top tiles
	int globaloffsety = (tileSize - (this->height()/2 - offsety) % tileSize)%tileSize;

	//num columns of tiles that fit right of central tile
	float tilesright = (float)(this->width()/2 + offsetx - tileSize)/tileSize;
	//num rows of tiles that fit under central tile
	float tilesbottom = (float)(this->height()/2 + offsety - tileSize)/tileSize;

	tilesToRender.left = xtile - ceil(tilesleft);
	tilesToRender.right = xtile + ceil(tilesright);
	tilesToRender.top =ytile - ceil(tilesup);
	tilesToRender.bottom = ytile + ceil(tilesbottom);
	tilesToRender.offsetx = globaloffsetx;
	tilesToRender.offsety = globaloffsety;
	tilesToRender.zoom = zoom;
}
/**
* Blits visible tiles buffer
*/
void cacaMap::updateBuffer()
{
	QPainter p(imgBuffer);
	imgBuffer->fill(Qt::gray);
	for (qint32 i= tilesToRender.left;i<= tilesToRender.right; i++)
	{
		for (qint32 j=tilesToRender.top ; j<= tilesToRender.bottom; j++)
		{
			QString x;
			QString y;
			//wrap around the tiles horizontally if i is outside [0,2^zoom]
			qint32 numtiles = 1<<tilesToRender.zoom;
			qint32 valx =((i<0)*numtiles + i%numtiles)%numtiles;
			x.setNum(valx);
			QPixmap image;
			int posx = (i-tilesToRender.left)*tileSize - tilesToRender.offsetx;
			int posy =  (j-tilesToRender.top)*tileSize - tilesToRender.offsety;
			//dont try to render tiles with y coords outside range
			//cause we cant do vertical wrapping!
			if (j>=0 && j<numtiles)
			{
				QString tileid = QString().setNum(tilesToRender.zoom) +"."+x+"."+QString().setNum(j);
				if (tileCache.contains(tileid))
				{
					//render the tile
					QDir::setCurrent(folder);
					//check path format (windows?)
					QString path= getTilePath(tilesToRender.zoom,valx) ;
					QString fileName = servermgr.fileName(j);
					QDir::setCurrent(path);
					QFile f(fileName);
					if (f.open(QIODevice::ReadOnly))
					{
						image.loadFromData(f.readAll());
						f.close();
					}
					else
					{
						cout<<"no file found "<<path.toStdString()<<fileName.toStdString()<<endl;
					}
				}
				//check if it's in the list of unavailable tiles
				else if (unavailableTiles.contains(tileid))
				{
					image = notAvailableTile;
				}
				//the tile is not cached so download it
				else
				{
					//check that the image hasnt been queued already
					if (!downloadQueue.contains(tileid))
					{
						tile t;
						t.zoom = tilesToRender.zoom;
						t.x = valx;
						t.y = j;
						t.url = servermgr.getTileUrl(tilesToRender.zoom,valx,j);
						//queue the image for download
						downloadQueue.insert(tileid,t);
					}
					//crop a tile from a lower zoom level and use it as a patch(a la google maps)
					//while the tile is downloading	
					image = getTilePatch(tilesToRender.zoom,valx,j,0,0,tileSize);
				}
				p.drawPixmap(posx,posy,image);
			}
		}
	}
	p.drawRect(0,0,width()-1, height()-1);
	if (!downloading)
	{
		downloadPicture();
	}
}
/**
* calls the following two functions
* @see cacaMap::updateTilesToRender
* @see cacaMap::updateBuffer
*/
void cacaMap::updateContent()
{
	updateTilesToRender();
	updateBuffer();
    update();
}

void cacaMap::clearCache() {
    // https://stackoverflow.com/questions/14754977/delete-all-files-in-a-directory
    qDebug() << "Removing everything";
    QDir dir("cache");
    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot);
    foreach(QString dirName, dir.entryList())
    {
       qDebug() << "Removing: " << dirName;
       QDir("cache/" + dirName).removeRecursively();
    }
    cacheSize = 0;
}
