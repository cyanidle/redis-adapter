## При включении в проджект файл тестов укажите переменную RSK_TEST_NAME
ADAPTER_LIB_DIR = $$PWD
ADAPTER_SRC_DIR = $$PWD/src
include(qtmodules.pri)
include(gtest_dependency.pri)
CONFIG += link_prl
CONFIG -= app_bundle gui 
QT += testlib
TEMPLATE = app
DEFINES += RADAPTER_SHARED_SRC=Q_DECL_IMPORT
CONFIG += console c++11
CONFIG += thread
SOURCES += \
    gtest_$${RSK_TEST_NAME}.cpp
HEADERS += \
    gtest_$${RSK_TEST_NAME}.h
DESTDIR = ../../tests-bin/gtest
OBJECTS_DIR = build
MOC_DIR = build
RCC_DIR = build
UI_DIR= build
##################### Подключение основной библиотеки проекта и библиотеки для тестов
LIBS += -L$$ADAPTER_LIB_DIR -lrsk-testlib
CONFIG(debug, debug|release){
    LIBS += -lradapter-sdkd
}
CONFIG(release, debug|release){
    LIBS += -lradapter-sdk
}
INCLUDEPATH += $$ADAPTER_SRC_DIR $$PWD/rsk-testlib