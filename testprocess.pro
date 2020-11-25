#-------------------------------------------------
#
# Project created by QtCreator 2018-09-06T04:45:00
#
#-------------------------------------------------

QT       += core gui
QT       += sql
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Paxgen
TEMPLATE = app


# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
CONFIG += config
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h \
    mainwindow_backup.h

FORMS    += mainwindow.ui

INCLUDEPATH +=/usr/local/include/opencv

LIBS += -L/usr/local/lib -lopencv_videoio -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lraspicam -lraspicam_cv -lopencv_imgproc

LIBS += -L/opt/vc/lib -lmmal -lmmal_core -lmmal_util

LIBS += -lwiringPi
