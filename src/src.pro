TEMPLATE = lib
QT += core \
    serialbus \
    serialport \
    sql \
    websockets \
    network

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

CONFIG += create_prl staticlib
DESTDIR = ..
DEFINES += RADAPTER_API=

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

win32: {
    LIBS += -lws2_32
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
