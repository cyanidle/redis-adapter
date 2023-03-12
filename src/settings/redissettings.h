#ifndef REDISSETTINGS_H
#define REDISSETTINGS_H
#include "settings.h"
#include "broker/interceptors/logginginterceptorsettings.h"

namespace Settings {
    struct RADAPTER_SHARED_SRC RedisServer : ServerInfo {
        typedef QMap<QString, RedisServer> Map;
        static Map &cacheMap() {
            static Map map{};
            return map;
        }
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        SERIAL_POST_INIT(cache)
        void cache() {
            cacheMap().insert(name, *this);
        }
    };

    struct RADAPTER_SHARED_SRC RedisConnector : Settings::SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, server_name)
        SERIAL_FIELD(quint16, db_index, 0)
        SERIAL_FIELD(quint16, ping_delay, 10000)
        SERIAL_FIELD(quint16, reconnect_delay, 1500)
        SERIAL_FIELD(quint16, max_command_errors, 3)
        SERIAL_FIELD(quint16, tcp_timeout, 1000)
        SERIAL_FIELD(quint16, command_timeout, 150)
        RedisServer server;
        SERIAL_POST_INIT(postInit)
        void postInit() {
            server = RedisServer::cacheMap().value(server_name);
        }
    };

    struct RADAPTER_SHARED_SRC RedisKeyEventSubscriber : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CONTAINER(QList, QString, keyEvents)
    };

    struct RADAPTER_SHARED_SRC RedisStreamBase : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, stream_key)
        SERIAL_FIELD(qint32, stream_size, 1000000u)
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
        SERIAL_FIELD_MAPPED(StartMode, start_from, startModesMap(), StartPersistentId)
        static QMap<QString, StartMode> &startModesMap() {
            static QMap<QString, StartMode> map {
                {"persistent_id", StartPersistentId},
                {"top", StartFromTop},
                {"first", StartFromFirst}
            };
            return map;
        }
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

    struct RADAPTER_SHARED_SRC RedisCacheConsumer : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, object_hash_key)
        SERIAL_FIELD(quint32, update_rate, 0) // 0 = no manual update
    };

    struct RADAPTER_SHARED_SRC RedisCacheProducer : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, object_hash_key)
    };
}

#endif // REDISSETTINGS_H
