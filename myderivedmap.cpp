#include "myderivedmap.h"
#include <QWidget>

using namespace std;

myDerivedMap::myDerivedMap(QWidget* parent):cacaMap(parent)
{
	mindistance = 0.025;
	animrate = 0.5;	
	
	hlayout = new QHBoxLayout;
	
	setLayout(hlayout);
}

myDerivedMap::~myDerivedMap()
{
	delete hlayout;
}

/**
Saves the screen coordinates of the last click
This is used for scrolling the map
@see myDerived::mouseMoveEvent()
*/
void myDerivedMap::mousePressEvent(QMouseEvent* e)
{
	mouseAnchor = e->pos();
}

/**
Calculates the length of the mouse drag and
translates it into a new coordinate, map is rerendered
*/
void myDerivedMap::mouseMoveEvent(QMouseEvent* e)
{
	QPoint delta = e->pos()- mouseAnchor;
	mouseAnchor = e->pos();
	longPoint p = myMercator::geoCoordToPixel(geocoords,zoom,tileSize);
	
	p.x-= delta.x();
	p.y-= delta.y();
	geocoords = myMercator::pixelToGeoCoord(p,zoom,tileSize);
	updateContent();
	update();
}

void myDerivedMap::mouseDoubleClickEvent(QMouseEvent* e)
{
	//do the zoom-in animation magic
	if (e->button() == Qt::LeftButton)
	{
		QPoint deltapx = e->pos() - QPoint(width()/2,height()/2);
		longPoint currpospx = myMercator::geoCoordToPixel(geocoords,zoom,tileSize);
		longPoint newpospx;
		newpospx.x = currpospx.x + deltapx.x();
		newpospx.y = currpospx.y + deltapx.y();
		destination = myMercator::pixelToGeoCoord(newpospx,zoom,tileSize);
        zoomAnim();
	}
	//do a simple zoom out for now
	else if (e->button() == Qt::RightButton)
	{
		zoomOut();
        if(scrollBarReady == true)
        {
            zoomScrollbar->setSliderPosition(zoom);
        }
		update();
	}
}

void myDerivedMap::zoomAnim()
{
    //float delta = buffzoomrate - 0.5;
    //if (delta > mindistance)
    //{
    //	QPointF deltaSpace = destination - geocoords;
    //	geocoords+=animrate*deltaSpace;
    //	buffzoomrate-= delta*animrate;
    //
    //}
	//you are already there
    //else
    //{
    geocoords = destination;
    buffzoomrate = 1.0;
    zoomIn();
    if(scrollBarReady == true)
    {
        zoomScrollbar->setSliderPosition(zoom);
    }
    //}
    updateContent();
	update();
}
void myDerivedMap::updateZoom(int newZoom)
{
	setZoom(newZoom);
	update();
}

void myDerivedMap::paintEvent(QPaintEvent *e)
{
	cacaMap::paintEvent(e);
}

void myDerivedMap::applyScrollbarSettings() {
    zoomScrollbar->setMaximum(maxZoom);
    zoomScrollbar->setMinimum(minZoom);
    zoomScrollbar->setSliderPosition(zoom);
    connect(zoomScrollbar, SIGNAL(valueChanged(int)),this, SLOT(updateZoom(int)));
    scrollBarReady = true;
}
