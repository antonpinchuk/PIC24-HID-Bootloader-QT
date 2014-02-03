#-------------------------------------------------
#
# Project created by QtCreator 2014-01-29T16:34:28
#
#-------------------------------------------------

QT       += core gui
QT       += network



greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

VERSION = 0.0.2.1402031630
TARGET   = viking-upd
TEMPLATE = app

DEFINES += APPLICATION_VERSION=\"\\\"$$VERSION\\\"\"

SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

RESOURCES += \
    res.qrc
