#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "myderivedmap.h"

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_serverComboBox_currentIndexChanged(int index);

    void on_zoomInBtn_clicked();

    void on_zoomOutBtn_clicked();

    void on_exitBtn_clicked();

private:
    Ui::MainWindow *ui;
    myDerivedMap* map = new myDerivedMap(this);

};

#endif // MAINWINDOW_H
