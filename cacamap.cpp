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
	cout<<"contructor"<<endl;
	cacheSize = 0;
	maxZoom = 18;
	minZoom = 0;
	folder = QDir::currentPath();
	loadCache();
	geocoords = QPointF(23.5,61.5);
	//geocoords = QPointF(0.0,0.0);
	downloading = false;
	tileFormat = ".png";
	tileSize = 256;
	QSize size(384,384);
	resize(size);
	zoom = 7;
	manager = new QNetworkAccessManager(this);
	loadingAnim.setFileName("loading.gif");
	loadingAnim.setScaledSize(QSize(tileSize,tileSize));
	loadingAnim.start();

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
		updateTilesToRender();
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
		updateTilesToRender();
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
		updateTilesToRender();
		return true;
	}
	return false;
}

/**
Saves the screen coordinates of the last click
This is used for scrolling the map
@see cacaMap::mouseMoveEvent()
*/
void cacaMap::mousePressEvent(QMouseEvent* e)
{
	mouseAnchor = e->pos();
}

/**
Calculates the lenght of the mouse drag and
translates it into a new coordinate, map is rerendered
*/
void cacaMap::mouseMoveEvent(QMouseEvent* e)
{
	QPoint delta = e->pos()- mouseAnchor;
	mouseAnchor = e->pos();
	
	qreal dx = - 180.0*(qreal)delta.x()/((1<<zoom)*tileSize);
	qreal dy =  180.0*(qreal)delta.y()/((1<<zoom)*tileSize);
	qreal  &x =  geocoords.rx();
	qreal  &y = geocoords.ry();

	x = ((x+dx)<-180)?360 + x+dx:x+dx;

	x = ((x+dx)>180)?-360 + x+dx:x+dx;
	
	y = ((y+dy)<-85)?-85:y+dy;

	y = ((y+dy)>85)?85:y+dy;

	updateTilesToRender();
	update();
}


/**
* Get URL of a specific %tile
* @param zoom zoom level
* @return string containing the url where the %tile image can be found.
*/
QString cacaMap::getTileUrl(int zoom, int x, int y)
{
	QString sz,sx,sy;
	sz.setNum(zoom);
	sx.setNum(x);
	sy.setNum(y);
	sy+=tileFormat;
	QString surl= QString("http://tile.openstreetmap.org/")+sz+"/"+sx+"/"+sy;
	return surl;

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
			QNetworkReply *reply = manager->get(request);
			connect(reply, SIGNAL(error(QNetworkReply::NetworkError)),this, SLOT(slotError(QNetworkReply::NetworkError)));
			connect(manager, SIGNAL(finished(QNetworkReply*)),this, SLOT(slotDownloadReady(QNetworkReply*)));
			connect(reply, SIGNAL(downloadProgress(qint64,qint64)),this, SLOT(slotDownloadProgress(qint64, qint64)));
		}
		else
		{
			//cout<<"no items in the queue"<<endl;
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
	QDir::setCurrent(folder);
	QDir dir;
	if (dir.cd("cache"))
	{
		if (dir.cd("osm"))
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
						cacheSize+= latitudes.at(k).size();
						QString name = zoomLevel+"."+lon+"."+lat;
						tileCache.insert(name,1);
					}
					dir.cdUp();//go back to zoom level folder
				}
				dir.cdUp();//go back to osm folder
			}
		}
		QDir::setCurrent(folder);
		cout<<"cache size "<<(float)cacheSize/1024/1024<<" MB"<<endl;
	}
}

/**
Slot to keep track of download progress
*/
void cacaMap::slotDownloadProgress(qint64 _bytesReceived, qint64 _bytesTotal)
{
}

/**
Slot that gets called everytime a %tile download request finishes
Saves image file to HDD, takes out item from download queue, and  adds item to cache list
*/
void cacaMap::slotDownloadReady(QNetworkReply * _reply)
{
	QNetworkReply::NetworkError error = _reply->error();
	
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
			
			//get image data
			QByteArray data = _reply->readAll();
			
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
			if (found)
			{
				tile nextItem = i.value();
				QString kk = i.key();
				QString zdir = QString().setNum(nextItem.zoom);
				QString xdir = QString().setNum(nextItem.x);
				QString tilefile = QString().setNum(nextItem.y)+tileFormat;

				QDir::setCurrent(folder);
				QDir dir;
				if (!dir.exists("cache"))
				{
					dir.mkdir("cache");
				}
				dir.cd("cache");
				if (!dir.exists("osm"))
				{
					dir.mkdir("osm");
				}
				dir.cd("osm");

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
				if (downloadQueue.remove(i.key())!= 1)
				{
					cout<<"item wasn't removed from queue"<<endl;
				}			
				
				//add it to cache
				tileCache.insert(kk,1);
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
			cout<<"no data"<<endl;
		}
		update();
	}
	else
	{
		cout<<"network error: "<<error<<endl;
	}
	_reply->deleteLater();
}
/**
Slot that gets called when theres is an network error
*/
void cacaMap::slotError(QNetworkReply::NetworkError _code)
{
	cout<<"some error "<<_code<<endl;
}
/**
Widget Resize event handler
Recalculates the range of visible tiles
*/
void cacaMap::resizeEvent(QResizeEvent* event)
{
	updateTilesToRender();
}

/**
Renders map based on range of visible tiles
*/
void cacaMap::renderMap(QPainter &p)
{
	for (qint32 i= tilesToRender.left;i<= tilesToRender.right; i++)
	{
		for (qint32 j=tilesToRender.top ; j<= tilesToRender.bottom; j++)
		{
			QString x;
			QString y;

			qint32 valx = (i<0)?((1<<zoom)+ i):i;
			valx = (valx<(1<<zoom))?valx:valx-(1<<zoom);
			x.setNum(valx);
			
			QImage image;
			int posx = (i-tilesToRender.left)*tileSize - tilesToRender.offsetx;
			int posy =  (j-tilesToRender.top)*tileSize - tilesToRender.offsety;
			QString tileid = QString().setNum(tilesToRender.zoom) +"."+x+"."+QString().setNum(j);
			if (tileCache.contains(tileid))
			{
				//render the tile
				QDir::setCurrent(folder);
				QString path =  "cache/osm/"+QString().setNum(tilesToRender.zoom) +"/"+x+"/";
				QString fileName = QString().setNum(j)+tileFormat;
				QDir::setCurrent(path);
				QFile f(fileName);
				if (f.open(QIODevice::ReadOnly))
				{
					
					image.loadFromData(f.readAll());
					f.close();
									}
				else
				{
					cout<<"no hay file "<<path.toStdString()<<endl;
				}
			}
			else
			{
				//check that the image hasnt been queued already
				if (!downloadQueue.contains(tileid))
				{
					tile t;
					t.zoom = tilesToRender.zoom;
					t.x = valx;
					t.y = j;
					t.url = getTileUrl(tilesToRender.zoom,valx,j);

					
					//queue the image for download
					downloadQueue.insert(tileid,t);

				}
				image = loadingAnim.currentImage();
				
			}
			p.drawImage(posx,posy,image);
			p.drawRect(posx,posy,tileSize, tileSize);
		}
	}
	if (!downloading)
	{
		downloadPicture();
	}
}
/**
Paint even handler
*/
void cacaMap::paintEvent(QPaintEvent* event)
{
	QPainter painter(this);
	renderMap(painter);
}

/**
destructor
*/
cacaMap::~cacaMap()
{
	delete manager;
	cout<<"destructor"<<endl;
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