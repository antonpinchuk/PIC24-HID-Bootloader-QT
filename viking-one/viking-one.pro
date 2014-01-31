#-------------------------------------------------
#
# Project created by QtCreator 2014-01-23T15:46:03
#
#-------------------------------------------------

QT += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

VERSION = 0.0.1.1401301400
TARGET = viking-one
TEMPLATE = app

DEFINES += APPLICATION_VERSION=\"\\\"$$VERSION\\\"\"

SOURCES += main.cpp\
        mainwindow.cpp \
    updatescheduler.cpp \
    updateavailabledialog.cpp

HEADERS  += mainwindow.h \
    updatescheduler.h \
    updateavailabledialog.h

FORMS    += mainwindow.ui \
    updateavailabledialog.ui

OTHER_FILES +=

RESOURCES += \
    res.qrc
