TEMPLATE = lib
QT += core \
    serialbus \
    serialport \
    sql \
    websockets \
    network \
    httpserver \
    concurrent \
    sql

QT -= gui

CONFIG(debug, debug|release){
    TARGET = radapter-sdkd
    OBJECTS_DIR = build/debug
    MOC_DIR = build/debug
}
CONFIG(release, debug|release){
    TARGET = radapter-sdk
    OBJECTS_DIR = build/release
    MOC_DIR = build/release
}
CONFIG -= debug_and_release
PRECOMPILED_HEADER += private/global.h
CONFIG += c++17 create_prl staticlib
DESTDIR = ..
DEFINES += RADAPTER_API= YAML_CPP_API=

# GCC shared library flags
gcc {
    COMPILER_VERSION = $$system($$QMAKE_CXX " -dumpversion")
    message("gcc version: $$COMPILER_VERSION")
    QMAKE_CXXFLAGS += -fpic
}

include($$PWD/../radapter_src.pri)

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

