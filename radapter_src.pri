QT += core \
    serialbus \
    serialport \
    sql \
    websockets \
    network \
    httpserver \
    concurrent
QT -= gui
PRECOMPILED_HEADER += $$PWD/src/private/global.h
CONFIG += c++17 create_prl
win32: {
    LIBS += -lws2_32
    QMAKE_CXXFLAGS += /bigobj
    g++:{
        ENV_PATH = $$(PATH)
        ENV_PATH_DIRS = $$split(ENV_PATH, ;)
        MINGW_DIRS = $$find(ENV_PATH_DIRS, mingw)
        MINGW_TOOLS_DIRS = $$find(MINGW_DIRS, Tools)
        MINGW_TOOLS = $$first(MINGW_TOOLS_DIRS)
        MINGW_LIBS_PATH = $$MINGW_TOOLS/../x86_64-w64-mingw32/lib
        MINGW_LIBS_PATH = $$clean_path($$MINGW_LIBS_PATH)
        message("mingw_libs: $$MINGW_LIBS_PATH")
        LIBS += -L$${MINGW_LIBS_PATH} -lws2_32
    }
}
include($$PWD/src/src.pri)
include($$PWD/headers.pri)
include($$PWD/src/lib/lib.pri)

RESOURCES += \
    $$PWD/radapter.qrc
