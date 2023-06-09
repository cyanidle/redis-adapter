TEMPLATE = app
QT += core \
    serialbus \
    serialport \
    sql \
    websockets \
    network \
    httpserver \
    concurrent

QT -= gui

TARGET = redis-adapter

DEFINES += RADAPTER_API=
CONFIG += c++17 console link_prl
CONFIG -= app_bundle

SOURCES += main.cpp
include($$PWD/../headers.pri)

LIBS += -L..
CONFIG(debug, debug|release){
    LIBS += -lradapter-sdkd
    OBJECTS_DIR = ../build/debug
    MOC_DIR = ../build/debug
}
CONFIG(release, debug|release){
    LIBS += -lradapter-sdk
    OBJECTS_DIR = ../build/release
    MOC_DIR = ../build/release
}
DESTDIR = ..

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

