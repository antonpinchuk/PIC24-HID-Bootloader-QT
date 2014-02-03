#-------------------------------------------------
#
# Project created by QtCreator 2014-01-23T15:46:03
#
#-------------------------------------------------

QT += core gui
QT += network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

VERSION = 0.0.2.1402032020
TARGET = viking-one
TEMPLATE = app

DEFINES += APPLICATION_VERSION=\"\\\"$$VERSION\\\"\"

INCLUDEPATH += ../
HEADERS += \
    mainwindow.h \
    updatescheduler.h \
    updateavailabledialog.h \
    ../HIDBootloader/Bootloader.h \
    ../HIDBootloader/Comm.h \
    ../HIDBootloader/DeviceData.h \
    ../HIDBootloader/Device.h \
    ../HIDBootloader/ImportExportHex.h

SOURCES += \
    main.cpp\
    mainwindow.cpp \
    updatescheduler.cpp \
    updateavailabledialog.cpp \
    ../HIDBootloader/Bootloader.cpp \
    ../HIDBootloader/Comm.cpp \
    ../HIDBootloader/DeviceData.cpp \
    ../HIDBootloader/Device.cpp \
    ../HIDBootloader/ImportExportHex.cpp

FORMS += \
    mainwindow.ui \
    updateavailabledialog.ui

OTHER_FILES +=

RESOURCES += \
    res.qrc

#-------------------------------------------------
# Add the correct HIDAPI library according to what
# OS is being used
#-------------------------------------------------
win32: LIBS += -L../HIDAPI/windows
macx: LIBS += -L../HIDAPI/mac
unix: !macx: LIBS += -L../HIDAPI/linux
LIBS += -lHIDAPI

#-------------------------------------------------
# Make sure to add the required libraries or
# frameoworks for the hidapi to work depending on
# what OS is being used
#-------------------------------------------------
macx: LIBS += -framework CoreFoundation -framework IOkit
win32: LIBS += -lSetupAPI
unix: !macx: LIBS += -lusb-1.0

#-------------------------------------------------
# Make sure output directory for object file and
# executable is in the correct subdirectory
#-------------------------------------------------
macx {
    DESTDIR = mac
    OBJECTS_DIR = mac
    MOC_DIR = mac
    UI_DIR = mac
    RCC_DIR = mac
}
unix: !macx {
    DESTDIR = linux
    OBJECTS_DIR = linux
    MOC_DIR = linux
    UI_DIR = linux
    RCC_DIR = linux
}
win32 {
    DESTDIR = windows
    OBJECTS_DIR = windows
    MOC_DIR = windows
    UI_DIR = windows
    RCC_DIR = windows
}
