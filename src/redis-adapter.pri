
SOURCES += \
    $$PWD/redis-adapter/connectors/modbusconnector.cpp \
    $$PWD/redis-adapter/connectors/mysqlconnector.cpp \
    $$PWD/redis-adapter/connectors/redisconnector.cpp \
    $$PWD/redis-adapter/connectors/websocketserverconnector.cpp \
    $$PWD/redis-adapter/commands/redis/rediscachecommands.cpp \
    $$PWD/redis-adapter/consumers/rediscacheconsumer.cpp \
    $$PWD/redis-adapter/consumers/rediskeyeventsconsumer.cpp \
    $$PWD/redis-adapter/consumers/redisstreamconsumer.cpp \
    $$PWD/redis-adapter/consumers/sqlkeyvaultconsumer.cpp \
    $$PWD/redis-adapter/factories/mysqlfactory.cpp \
    $$PWD/redis-adapter/factories/rediscachefactory.cpp \
    $$PWD/redis-adapter/factories/redispubsubfactory.cpp \
    $$PWD/redis-adapter/factories/redisstreamfactory.cpp \
    $$PWD/redis-adapter/factories/sqlarchivefactory.cpp \
    $$PWD/redis-adapter/factories/websocketclientfactory.cpp \
    $$PWD/redis-adapter/formatters/archivequeryformatter.cpp \
    $$PWD/redis-adapter/formatters/keyvaultresultformatter.cpp \
    $$PWD/redis-adapter/formatters/modbusformatter.cpp \
    $$PWD/redis-adapter/formatters/rediskeyeventformatter.cpp \
    $$PWD/redis-adapter/formatters/redisqueryformatter.cpp \
    $$PWD/redis-adapter/formatters/redisstreamentryformatter.cpp \
    $$PWD/redis-adapter/formatters/sqlqueryfieldsformatter.cpp \
    $$PWD/redis-adapter/formatters/sqlqueryformatter.cpp \
    $$PWD/redis-adapter/formatters/sqlresultformatter.cpp \
    $$PWD/redis-adapter/formatters/streamentriesmapformatter.cpp \
    $$PWD/redis-adapter/formatters/timeformatter.cpp \
    $$PWD/redis-adapter/launcher.cpp \
    $$PWD/redis-adapter/producers/producerfilter.cpp \
    $$PWD/redis-adapter/producers/rediscacheproducer.cpp \
    $$PWD/redis-adapter/producers/redisstreamproducer.cpp \
    $$PWD/redis-adapter/producers/sqlarchiveproducer.cpp \
    $$PWD/redis-adapter/localization.cpp \
    $$PWD/redis-adapter/localstorage.cpp \
    $$PWD/redis-adapter/radapterlogging.cpp \
    $$PWD/redis-adapter/replies/redis/redisreplies.cpp \
    $$PWD/redis-adapter/settings/mysqlserializable.cpp \
    $$PWD/redis-adapter/workers/modbusslaveworker.cpp \
    $$PWD/redis-adapter/settings/modbussettings.cpp \
    $$PWD/redis-adapter/settings/redissettings.cpp \
    $$PWD/redis-adapter/settings/settings.cpp

HEADERS += \
    $$PWD/redis-adapter/connectors/modbusconnector.h \
    $$PWD/redis-adapter/connectors/mysqlconnector.h \
    $$PWD/redis-adapter/connectors/redisconnector.h \
    $$PWD/redis-adapter/connectors/websocketserverconnector.h \
    $$PWD/redis-adapter/commands/redis/rediscachecommands.h \
    $$PWD/redis-adapter/consumers/rediscacheconsumer.h \
    $$PWD/redis-adapter/consumers/rediskeyeventsconsumer.h \
    $$PWD/redis-adapter/consumers/redisstreamconsumer.h \
    $$PWD/redis-adapter/consumers/sqlkeyvaultconsumer.h \
    $$PWD/redis-adapter/factories/mysqlfactory.h \
    $$PWD/redis-adapter/factories/rediscachefactory.h \
    $$PWD/redis-adapter/factories/redispubsubfactory.h \
    $$PWD/redis-adapter/factories/redisstreamfactory.h \
    $$PWD/redis-adapter/factories/sqlarchivefactory.h \
    $$PWD/redis-adapter/factories/websocketclientfactory.h \
    $$PWD/redis-adapter/formatters/archivequeryformatter.h \
    $$PWD/redis-adapter/formatters/keyvaultresultformatter.h \
    $$PWD/redis-adapter/formatters/modbusformatter.h \
    $$PWD/redis-adapter/formatters/rediskeyeventformatter.h \
    $$PWD/redis-adapter/formatters/redisqueryformatter.h \
    $$PWD/redis-adapter/formatters/redisstreamentryformatter.h \
    $$PWD/redis-adapter/formatters/sqlqueryfieldsformatter.h \
    $$PWD/redis-adapter/formatters/sqlqueryformatter.h \
    $$PWD/redis-adapter/formatters/sqlresultformatter.h \
    $$PWD/redis-adapter/formatters/streamentriesmapformatter.h \
    $$PWD/redis-adapter/formatters/timeformatter.h \
    $$PWD/redis-adapter/formatters/wordoperations.h \
    $$PWD/redis-adapter/include/modbuskeys.h \
    $$PWD/redis-adapter/include/redismessagekeys.h \
    $$PWD/redis-adapter/include/sqlarchivefields.h \
    $$PWD/redis-adapter/include/sqlkeyvaultfields.h \
    $$PWD/redis-adapter/launcher.h \
    $$PWD/redis-adapter/producers/producerfilter.h \
    $$PWD/redis-adapter/producers/rediscacheproducer.h \
    $$PWD/redis-adapter/producers/redisstreamproducer.h \
    $$PWD/redis-adapter/producers/sqlarchiveproducer.h \
    $$PWD/redis-adapter/localization.h \
    $$PWD/redis-adapter/localstorage.h \
    $$PWD/redis-adapter/radapterlogging.h \
    $$PWD/redis-adapter/replies/redis/redisreplies.h \
    $$PWD/redis-adapter/settings/mysqlserializable.h \
    $$PWD/redis-adapter/workers/modbusslaveworker.h \
    $$PWD/redis-adapter/settings/modbussettings.h \
    $$PWD/redis-adapter/settings/redissettings.h \
    $$PWD/redis-adapter/settings/settings.h


include($$PWD/lib/lib.pri)