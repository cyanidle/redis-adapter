## При включении в проджект файл тестов укажите переменную RSK_TEST_NAME
ADAPTER_LIB_DIR = $$PWD
ADAPTER_SRC_DIR = $$PWD/src
include(qtmodules.pri)
QT += testlib
CONFIG += qt console warn_on depend_includepath testcase link_prl
CONFIG -= app_bundle gui
TEMPLATE = app
DEFINES += RADAPTER_SHARED_SRC=Q_DECL_IMPORT
SOURCES += \
    tst_$${RSK_TEST_NAME}.cpp
HEADERS += \
    tst_$${RSK_TEST_NAME}.h
DESTDIR = ../../tests-bin/qtest
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
