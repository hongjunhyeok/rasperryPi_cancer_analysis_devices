#include "mainwindow.h"
#include <QApplication>
#include <QPushButton>
#include <qwidget.h>
#include <QDesktopWidget>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QDesktopWidget dw;
    MainWindow w;

    int x=dw.width()*0.97;
    int y = dw.height();

    w.setFixedSize(x,y);

    w.show();

    return a.exec();

}
