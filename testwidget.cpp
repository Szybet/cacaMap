/*
Copyright 2010 Jean Fairlie jmfairlie@gmail.com

This program  is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/
#include "testwidget.h"
#include <QWidget>

testWidget::testWidget(QWidget* parent):QWidget(parent)
{
	map = new myDerivedMap(this);

	

	vlayout = new QVBoxLayout(this);
	combo = new QComboBox(this);
	populateCombo();
	vlayout->addWidget(combo);

	
	connect(combo, SIGNAL(currentIndexChanged(int)),this, SLOT(setServer(int)));

	vlayout->addWidget(map);


	setLayout(vlayout);

	QSize size(384,384);
	resize(size);
}

void testWidget::populateCombo()
{

	QStringList l = map->getServerNames();

	for (int i=0; i<l.size(); i++)
	{
		combo->addItem(l.at(i),QVariant(i));
	}
}

void testWidget::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
}

testWidget::~testWidget()
{
	delete map;
	delete vlayout;
}



void testWidget::setServer(int index)
{
	map->setServer(index);
}


