#ifndef DERIVEDMAP_H
#define DERIVEDMAP_H
#include "cacamap.h"

class myDerivedMap: public cacaMap
{

Q_OBJECT
public:
	myDerivedMap(QWidget* _parent=0);
	~myDerivedMap();
    QScrollBar* zoomScrollbar;

public slots:
    // Thats a thing if someone wants to use an external scroll bar. not usable on ereaders
    void applyScrollbarSettings();

protected:
	void paintEvent(QPaintEvent *);
	void mousePressEvent(QMouseEvent*);
	void mouseMoveEvent(QMouseEvent*);
	void mouseDoubleClickEvent(QMouseEvent*);
private:
	QPoint mouseAnchor;/**< used to keep track of the last mouse click location.*/
	QHBoxLayout * hlayout;
	
	QPointF destination; /**< used for dblclick+zoom animations */
	float mindistance;/**< used to identify the end of the animation*/
	float animrate; 

    bool scrollBarReady = false;
protected slots:
	void zoomAnim();
	void updateZoom(int);
};
#endif


