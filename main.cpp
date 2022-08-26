#include "mainwindow.h"

#include <QApplication>
#include <QtWidgets>

int main (int argc, char **argv)
{
	QApplication a(argc, argv);

    QDir::setCurrent(QCoreApplication::applicationDirPath());
    MainWindow MainWindownew;
    MainWindownew.show();
	return a.exec();
}
