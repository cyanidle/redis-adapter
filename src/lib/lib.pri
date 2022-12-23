HEADERS += \
    $$PWD/hiredis/adapters/qt.h \
    $$PWD/hiredis/alloc.h \
    $$PWD/hiredis/async.h \
    $$PWD/hiredis/async_private.h \
    $$PWD/hiredis/dict.h \
    $$PWD/hiredis/fmacros.h \
    $$PWD/hiredis/hiredis.h \
    $$PWD/hiredis/hiredis_ssl.h \
    $$PWD/hiredis/net.h \
    $$PWD/hiredis/read.h \
    $$PWD/hiredis/sds.h \
    $$PWD/hiredis/sdsalloc.h \
    $$PWD/hiredis/sockcompat.h \
    $$PWD/hiredis/win32.h \
    $$PWD/modbus/modbusclient.h \
    $$PWD/modbus/modbusdevicesgate.h \
    $$PWD/modbus/modbusfactory.h \
    $$PWD/modbus/modbusquery.h \
    $$PWD/modbus/modbusscheduler.h \
    $$PWD/modbus/modbusdeviceinfo.h \
    $$PWD/mysql/mysqlclient.h \
    $$PWD/websocket/websocketclient.h \
    $$PWD/websocket/websocketconstants.h \
    $$PWD/websocket/websocketserver.h 


SOURCES += \
    $$PWD/hiredis/alloc.c \
    $$PWD/hiredis/async.c \
    $$PWD/hiredis/dict.c \
    $$PWD/hiredis/hiredis.c \
    $$PWD/hiredis/net.c \
    $$PWD/hiredis/read.c \
    $$PWD/hiredis/sds.c \
    $$PWD/hiredis/sockcompat.c \
    $$PWD/modbus/modbusclient.cpp \
    $$PWD/modbus/modbusdevicesgate.cpp \
    $$PWD/modbus/modbusfactory.cpp \
    $$PWD/modbus/modbusquery.cpp \
    $$PWD/modbus/modbusscheduler.cpp \
    $$PWD/modbus/modbusdeviceinfo.cpp \
    $$PWD/mysql/mysqlclient.cpp \
    $$PWD/websocket/websocketclient.cpp \
    $$PWD/websocket/websocketserver.cpp 

INCLUDEPATH += \
    $$PWD \
    $$PWD/googletest

include($$PWD/radapter-broker/radapter-broker.pri)
