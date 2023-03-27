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
        FIELDS(name)
        RequiredField<QString> name;
        POST_UPDATE {
            cacheMap().insert(name, *this);
        }
    };

    struct RADAPTER_API RedisConnector : Settings::SerializableSettings {
        Q_GADGET
        FIELDS(worker,
               server_name,
               db_index,
               ping_delay,
               reconnect_delay,
               max_command_errors,
               tcp_timeout,
               command_timeout)
        RequiredField<Radapter::WorkerSettings> worker;
        NonRequiredField<QString> server_name;
        NonRequiredField<quint16> db_index{0};
        NonRequiredField<quint16> ping_delay{10000};
        NonRequiredField<quint16> reconnect_delay{1500};
        NonRequiredField<quint16> max_command_errors{3};
        NonRequiredField<quint16> tcp_timeout{1000};
        NonRequiredField<quint16> command_timeout{150};

        RedisServer server;
        POST_UPDATE {
            server = RedisServer::cacheMap().value(server_name);
        }
    };

    struct RADAPTER_API RedisKeyEventSubscriber : RedisConnector {
        Q_GADGET
        FIELDS(keyEvents)
        RequiredSequence<QString> keyEvents;
    };

    struct RADAPTER_API RedisStreamBase : RedisConnector {
        Q_GADGET
        FIELDS(stream_key, stream_size)
        RequiredField<QString> stream_key;
        RequiredField<quint32> stream_size{1000000u};
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
        Q_GADGET
        FIELDS(start_from)
        using StartFromField = Serializable::Validated<NonRequiredField<StartMode>>::With<RedisStreamConsumer>;
        StartFromField start_from{StartPersistentId};
    };
    struct RADAPTER_API RedisStreamGroupConsumer : RedisStreamConsumer {
        Q_GADGET
        FIELDS(consumer_group_name, start_from_last_unread)
        RequiredField<QString> consumer_group_name;
        NonRequiredField<bool> start_from_last_unread{true};
    };
    struct RADAPTER_API RedisStreamProducer : RedisStreamBase {
        Q_GADGET
    };

    struct RADAPTER_API RedisCacheConsumer : RedisConnector {
        Q_GADGET
        FIELDS(object_hash_key, update_rate)
        NonRequiredField<QString> object_hash_key;
        NonRequiredField<quint32> update_rate{600};
    };

    struct RADAPTER_API RedisCacheProducer : RedisConnector {
        Q_GADGET
        FIELDS(object_hash_key)
        RequiredField<QString> object_hash_key;
    };
}

#endif // REDISSETTINGS_H
