TEMPLATE = lib
QT += core
QT -= gui
CONFIG -= app_bundle
CONFIG += c++11 console
CONFIG += create_prl
SOURCES+=$$PWD/validatorsplugin.cpp
HEADERS+=$$PWD/validatorsplugin.h
DEFINES+=RADAPTER_API=Q_DECL_EXPORT
include($$PWD/../../headers.pri)
DESTDIR = ..
