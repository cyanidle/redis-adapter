#ifndef REDISSETTINGS_H
#define REDISSETTINGS_H
#include "settings.h"
#include "radapter-broker/debugging/logginginterceptorsettings.h"

namespace Settings {
    struct RADAPTER_SHARED_SRC RedisServer : ServerInfo {
        typedef QMap<QString, RedisServer> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        SERIAL_POST_INIT(cache)
        void cache() {
            cacheMap.insert(name, *this);
        }
    };

    struct RADAPTER_SHARED_SRC RedisConnector : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(quint16, db_index, 0)
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, target_server_name)
        SERIAL_FIELD_PTR(Radapter::LoggingInterceptorSettings, log_jsons, DEFAULT)
    };

    struct RADAPTER_SHARED_SRC RedisKeyEventSubscriber : Serializer::SerializableGadget {
        typedef QMap<QString, RedisKeyEventSubscriber> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, source_server_name)
        SERIAL_FIELD_PTR(Radapter::LoggingInterceptorSettings, log_jsons, DEFAULT)
        RedisServer source_server;
        SERIAL_CONTAINER(QList, QString, keyEvents)
        SERIAL_POST_INIT(postInit)
        void postInit() {
            source_server = RedisServer::cacheMap.value(source_server_name);
            cacheMap.insert(worker.name, *this);
        }
    };

    enum RedisStreamMode {
        RedisStreamConsumer = 0,
        RedisStreamProducer,
        RedisStreamConsumerGroups
    };

    enum RedisConsumerStartMode {
        RedisStartPersistentId = 0,
        RedisStartFromTop,
        RedisStartFromFirst,
        RedisStartFromLastUnread
    };

    struct RADAPTER_SHARED_SRC RedisStreamBase : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, stream_key)
        SERIAL_FIELD(qint32, stream_size, 10000)
    };
    struct RADAPTER_SHARED_SRC RedisStreamConsumer : RedisStreamBase {
        enum StartMode {
            StartPersistentId = 0,
            StartFromTop,
            StartFromFirst
        };
        Q_ENUM(StartMode)
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD_MAPPED(StartMode, start_from, startModesMap, StartPersistentId)
        static QMap<QString, StartMode> startModesMap;
    };
    struct RADAPTER_SHARED_SRC RedisStreamGroupConsumer : RedisStreamConsumer {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, consumer_group_name)
        SERIAL_FIELD(bool, start_from_last_unread, true)
    };
    struct RADAPTER_SHARED_SRC RedisStreamProducer : RedisStreamBase {
        Q_GADGET
        IS_SERIALIZABLE
    };

    struct RADAPTER_SHARED_SRC RedisStream : Serializer::SerializableGadget {
        typedef QMap<QString, RedisStream> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_CUSTOM(RedisStreamMode, mode, initMode, readMode)
        SERIAL_FIELD(QString, target_server, QString())
        RedisServer target;
        SERIAL_FIELD(QString, source_server, QString())
        RedisServer source;
        SERIAL_FIELD(QString, stream_key)
        SERIAL_FIELD_PTR(Radapter::LoggingInterceptorSettings, log_jsons, DEFAULT)
        SERIAL_FIELD(qint32, stream_size, 10000)
        SERIAL_FIELD(QString, consumer_group_name, "")
        SERIAL_CUSTOM(RedisConsumerStartMode, start_from, initStartMode, readStartMode, RedisStartFromTop)
        SERIAL_POST_INIT(postInit)
        void postInit() {
            if (!target_server.isEmpty()) {
                target = RedisServer::cacheMap.value(target_server);
            } else if (!source_server.isEmpty()) {
                source = RedisServer::cacheMap.value(source_server);
            }
            cacheMap.insert(worker.name, *this);
        }

        bool initStartMode(const QVariant &src) {
            auto strMode = src.toString();
            if (strMode == "persistent_id") {
                start_from = RedisConsumerStartMode::RedisStartPersistentId;
            } else if (strMode == "top") {
                start_from = RedisConsumerStartMode::RedisStartFromTop;
            } else if (strMode == "first") {
                start_from = RedisConsumerStartMode::RedisStartFromFirst;
            } else {
                return false;
            }
            return true;
        }

        QVariant readStartMode() const {
            if (start_from == RedisConsumerStartMode::RedisStartPersistentId) {
                return "persistent_id";
            } else if (start_from == RedisConsumerStartMode::RedisStartFromTop) {
                return "top";
            } else if (start_from == RedisConsumerStartMode::RedisStartFromFirst) {
                return "first";
            } else {
                return "persistent_id";
            }
        }

        bool initMode(const QVariant &src) {
            auto modeStr = src.toString().toLower();
            if (modeStr == "consumer") {
                mode = RedisStreamMode::RedisStreamConsumer;
                return true;
            } else if (modeStr == "producer") {
                mode = RedisStreamMode::RedisStreamProducer;
                return true;
            }
            return false;
        }

        QVariant readMode() const{
            if (mode == RedisStreamMode::RedisStreamConsumer) {
                return "consumer";
            } else if (mode == RedisStreamMode::RedisStreamProducer) {
                return "producer";
            } else  if (mode == RedisStreamMode::RedisStreamConsumerGroups){
                return "consumergroups";
            }
            return "unknown";
        }
    };

    struct RADAPTER_SHARED_SRC RedisCache : RedisConnector {
        typedef QMap<QString, RedisCache> Map;
        static Map cacheMap;
        enum Mode {
            NotSet = 0,
            Consumer,
            Producer
        };
        Q_ENUM(Mode)
        Q_GADGET
        IS_SERIALIZABLE
        RedisServer target_server;
        SERIAL_FIELD(QString, index_key)
        SERIAL_FIELD_MAPPED(Mode, mode, modeMap)
        SERIAL_POST_INIT(postInit)
        static QMap<QString, Mode> modeMap;
        void postInit() {
            target_server = RedisServer::cacheMap.value(target_server_name);
            cacheMap.insert(worker.name, *this);
        }
    };

}

#endif // REDISSETTINGS_H
