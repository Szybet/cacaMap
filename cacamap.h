/*
Copyright 2010 Jean Fairlie jmfairlie@gmail.com

CacaMap is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


/** @file cacamap.h
* CacaMap is a Simple Qt OSM Map Widget 
*/

#ifndef CACAMAP_H
#define CACAMAP_H
#include <QtWidgets>
#include <QtNetwork>
#include <iostream>
#include <vector>
#include "servermanager.h"

/**
* The quint32 version of QPoint
*/

struct longPoint
{
	quint32 x;/**< x coord. */
	quint32 y;/**< y coord.*/
	longPoint(quint32,quint32);
	longPoint();
};

/**
Helper struct that handles coordinate transformations
*/
struct myMercator
{
	static longPoint geoCoordToPixel(QPointF const &,int , int);
	static QPointF pixelToGeoCoord(longPoint const &, int, int);
};
/**
* Struct to define a range of consecutive tiles
* It's used to identify which tiles are visible and need to be rendered/downlaoded
* @see cacaMap::updateTilesToRender()
*/
struct tileSet
{
	int zoom;/**< zoom level.*/
	qint32 top;/**< topmost row.*/
	qint32 bottom;/**< bottommost row.*/
	qint32 left;/**< leftmostcolumn. */
	qint32 right;/**< rightmost column. */
	int offsetx;/**< horizontal offset needed to align the tiles in the wiget.*/
	int offsety;/**< vertical offset needed to align the tiles in the widget.*/
};

/**
* Used to represent a specific %tile
* @see cacaMap::tileCache
*/
struct tile
{
	int zoom;/**< zoom level.*/
	qint32 x;/**< colum number.*/
	qint32 y;/**< row number.*/
	QString  url;/**<used to identify the %tile when it finishes downloading.*/
};
/**
* maximum space allowed for caching tiles
*/
#define CACHE_MAX 1*1024*1024 //1MB
/**
Main map widget
*/


class cacaMap : public QWidget
{

Q_OBJECT

public:	
	cacaMap(QWidget * _parent=0);

	virtual ~cacaMap();
	void setGeoCoords(QPointF);
	bool zoomIn();
	bool zoomOut();
	bool setZoom(int level);
	QPointF getGeoCoords();
	QStringList getServerNames();
	void setServer(int);
	int getZoom();
    quint32 cacheSize;/**< current %tile cache size in bytes. */
    void clearCache();
    const QString cacheDirPath = "/app-temp/cache/";

private:
	QNetworkAccessManager *manager;/**< manages http requests. */
	tileSet tilesToRender;/**< range of visible tiles. */
	QHash<QString,int> tileCache;/**< list of cached tiles (in HDD). */
	QHash<QString,tile> downloadQueue;/**< list of tiles waiting to be downloaded. */
	QHash<QString,int> unavailableTiles;/**< list of tiles that were not found on the server.*/
	bool downloading;/**< flag that indicates if there is a download going on. */
	QString folder;/**< root application folder. */
	QMovie loadingAnim;/**< to show a 'loading' animation for yet unavailable tiles. */
	QPixmap notAvailableTile;
	servermanager servermgr;	

	void renderMap(QPainter &);
	void downloadPicture();
	void loadCache();
	QString getTilePath(int, qint32);
	QPixmap getTilePatch(int,quint32,quint32,int,int,int);

protected:
	int zoom;/**< Map zoom level. */
	int minZoom;/**< Minimum zoom level (farthest away).*/
	int maxZoom;/**< Maximum zoom level (closest).*/

	int tileSize; /**< size in px of the square %tile. */
	//check QtMobility QGeoCoordinate
	QPointF geocoords; /**< current longitude and latitude. */
	QPixmap* imgBuffer;
	QPixmap tmpbuff;
	float buffzoomrate;

	bool bufferDirty; /**< image buffer needs to be updated. */	
	void resizeEvent(QResizeEvent*);
	void paintEvent(QPaintEvent *);
	void updateTilesToRender();
	void updateBuffer();
	void updateContent();

protected slots:
	void slotDownloadProgress(qint64, qint64);
	void slotDownloadReady(QNetworkReply *);
	void slotError(QNetworkReply::NetworkError);
};
#endif
