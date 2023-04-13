SOURCES+= \
   $$PWD/launcher.cpp \
   $$PWD/initialization.cpp \
   $$PWD/radapterlogging.cpp \
   $$PWD/localstorage.cpp \
   $$PWD/localization.cpp
HEADERS+= \
   $$PWD/localstorage.h \
   $$PWD/radapterconfig.h \
   $$PWD/localization.h \
   $$PWD/initialization.h \
   $$PWD/launcher.h \
   $$PWD/radapterlogging.h

include($$PWD/interceptors/interceptors.pri)
include($$PWD/plugins/plugins.pri)
include($$PWD/settings-parsing/settings-parsing.pri)
include($$PWD/replies/replies.pri)
include($$PWD/websocket/websocket.pri)
include($$PWD/utils/utils.pri)
include($$PWD/private/private.pri)
include($$PWD/include/include.pri)
include($$PWD/producers/producers.pri)
include($$PWD/jsondict/jsondict.pri)
include($$PWD/consumers/consumers.pri)
include($$PWD/raw_sockets/raw_sockets.pri)
include($$PWD/templates/templates.pri)
include($$PWD/modbus/modbus.pri)
include($$PWD/serializable/serializable.pri)
include($$PWD/mysql/mysql.pri)
include($$PWD/validators/validators.pri)
include($$PWD/commands/commands.pri)
include($$PWD/connectors/connectors.pri)
include($$PWD/broker/broker.pri)
include($$PWD/filters/filters.pri)
include($$PWD/routed_object/routed_object.pri)
include($$PWD/settings/settings.pri)
include($$PWD/formatting/formatting.pri)
include($$PWD/async_context/async_context.pri)