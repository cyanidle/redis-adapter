#ifndef REDISSETTINGS_H
#define REDISSETTINGS_H
#include "settings.h"
#include "broker/interceptors/logginginterceptorsettings.h"

namespace Settings {
    struct RADAPTER_API RedisServer : ServerInfo {
        typedef QMap<QString, RedisServer> Map;
        static Map &cacheMap() {
            static Map map{};
            return map;
        }
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<QString>, name)
        POST_UPDATE {
            cacheMap().insert(name, *this);
        }
    };

    struct RADAPTER_API RedisConnector : Settings::SerializableSettings {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<Radapter::WorkerSettings>, worker)
        FIELD(NonRequired<QString>, server_name)
        FIELD(NonRequired<quint16>, db_index, {0})
        FIELD(NonRequired<quint16>, ping_delay, {10000})
        FIELD(NonRequired<quint16>, reconnect_delay, {1500})
        FIELD(NonRequired<quint16>, max_command_errors, {3})
        FIELD(NonRequired<quint16>, tcp_timeout, {1000})
        FIELD(NonRequired<quint16>, command_timeout, {150})

        RedisServer server;
        POST_UPDATE {
            server = RedisServer::cacheMap().value(server_name);
        }
    };

    struct RADAPTER_API RedisKeyEventSubscriber : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(RequiredSequence<QString>, keyEvents)
    };

    struct RADAPTER_API RedisStreamBase : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<QString>, stream_key)
        FIELD(Required<quint32>, stream_size, {1000000u})
    };

    struct RADAPTER_API RedisStreamConsumer : RedisStreamBase {
        enum StartMode {
            StartPersistentId = 0,
            StartFromTop,
            StartFromFirst
        };
        static bool validate(QVariant &target) {
            auto asStr = target.toString().toLower();
            if (asStr == "persistent_id") {
                target.setValue(StartPersistentId);
            } else if (asStr == "from_top") {
                target.setValue(StartFromTop);
            } else if (asStr == "from_first") {
                target.setValue(StartFromFirst);
            } else {
                return false;
            }
            return true;
        }
        Q_ENUM(StartMode)
        using StartFrom = Serializable::Validated<NonRequired<StartMode>>::With<RedisStreamConsumer>;

        Q_GADGET
        IS_SERIALIZABLE
        FIELD(StartFrom, start_from, {StartPersistentId})
    };
    struct RADAPTER_API RedisStreamGroupConsumer : RedisStreamConsumer {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<QString>, consumer_group_name)
        FIELD(NonRequired<bool>, start_from_last_unread, {true})
    };
    struct RADAPTER_API RedisStreamProducer : RedisStreamBase {
        Q_GADGET
        IS_SERIALIZABLE
    };

    struct RADAPTER_API RedisCacheConsumer : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(NonRequired<QString>, object_hash_key)
        FIELD(NonRequired<quint32>, update_rate, {600})
    };

    struct RADAPTER_API RedisCacheProducer : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<QString>, object_hash_key)
    };
}

#endif // REDISSETTINGS_H
