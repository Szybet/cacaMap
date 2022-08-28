#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "myderivedmap.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Stylesheet
    QFile stylesheetFile("/etc/eink.qss");
    stylesheetFile.open(QFile::ReadOnly);
    this->setStyleSheet(stylesheetFile.readAll());
    stylesheetFile.close();

    QStringList l = map->getServerNames();
    for (int i=0; i<l.size(); i++)
    {
        ui->serverComboBox->addItem(l.at(i),QVariant(i));
    }

    ui->mapLayout->addWidget(map);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_serverComboBox_currentIndexChanged(int index)
{
    map->setServer(index);
}

void MainWindow::on_zoomInBtn_clicked()
{
    map->zoomIn();
}

void MainWindow::on_zoomOutBtn_clicked()
{
    map->zoomOut();
}

void MainWindow::on_exitBtn_clicked()
{
    map->clearCache();
    map->deleteLater();
    map->close();
    QApplication::quit();
}

void MainWindow::on_refreshBtn_clicked()
{
    // Refreshes eInk screen
    this->repaint();
}
