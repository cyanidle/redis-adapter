SOURCES+= \
   $$PWD/initialization.cpp \
   $$PWD/launcher.cpp \
   $$PWD/localization.cpp \
   $$PWD/localstorage.cpp \
   $$PWD/radapterconfig.cpp \
   $$PWD/radapterlogging.cpp
HEADERS+= \
   $$PWD/initialization.h \
   $$PWD/launcher.h \
   $$PWD/localization.h \
   $$PWD/localstorage.h \
   $$PWD/radapterconfig.h \
   $$PWD/radapterlogging.h

include($$PWD/async_context/async_context.pri)
include($$PWD/broker/broker.pri)
include($$PWD/commands/commands.pri)
include($$PWD/connectors/connectors.pri)
include($$PWD/consumers/consumers.pri)
include($$PWD/filters/filters.pri)
include($$PWD/formatting/formatting.pri)
include($$PWD/httpserver/httpserver.pri)
include($$PWD/include/include.pri)
include($$PWD/interceptors/interceptors.pri)
include($$PWD/jsondict/jsondict.pri)
include($$PWD/modbus/modbus.pri)
include($$PWD/mysql/mysql.pri)
include($$PWD/plugins/plugins.pri)
include($$PWD/private/private.pri)
include($$PWD/producers/producers.pri)
include($$PWD/raw_sockets/raw_sockets.pri)
include($$PWD/replies/replies.pri)
include($$PWD/routed_object/routed_object.pri)
include($$PWD/serializable/serializable.pri)
include($$PWD/settings/settings.pri)
include($$PWD/settings-parsing/settings-parsing.pri)
include($$PWD/templates/templates.pri)
include($$PWD/utils/utils.pri)
include($$PWD/validators/validators.pri)
include($$PWD/websocket/websocket.pri)