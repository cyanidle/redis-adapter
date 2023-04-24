#ifndef REDISSETTINGS_H
#define REDISSETTINGS_H
#include "settings.h"

namespace Settings {

    struct RADAPTER_API RedisServer : ServerInfo {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<QString>, name)

        void postUpdate() override;
    };

    struct RADAPTER_API RedisConnector : Serializable {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<Worker>, worker)
        FIELD(Required<QString>, server_name)
        FIELD(HasDefault<quint16>, db_index, 0)
        FIELD(HasDefault<quint16>, ping_delay, 10000)
        FIELD(HasDefault<quint16>, reconnect_delay, 1500)
        FIELD(HasDefault<quint16>, max_command_errors, 3)
        FIELD(HasDefault<quint16>, tcp_timeout, 1000)
        FIELD(HasDefault<quint16>, command_timeout, 150)

        RedisServer server;
        void postUpdate() override;
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
        FIELD(HasDefault<quint32>, stream_size, 1000000u)
    };

    struct RADAPTER_API RedisStreamConsumer : RedisStreamBase {
        enum StartMode {
            StartPersistentId = 0,
            StartFromTop,
            StartFromFirst
        };
        Q_ENUM(StartMode)
        static bool validate(QVariant &target, const QVariantList &args, QVariant &state);
        using StartFrom = ::Serializable::Validated<HasDefault<StartMode>>::With<RedisStreamConsumer>;

        Q_GADGET
        IS_SERIALIZABLE
        FIELD(StartFrom, start_from, StartPersistentId)
    };
    struct RADAPTER_API RedisStreamGroupConsumer : RedisStreamConsumer {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Required<QString>, consumer_group_name)
        FIELD(HasDefault<bool>, start_from_last_unread, true)
    };
    struct RADAPTER_API RedisStreamProducer : RedisStreamBase {
        Q_GADGET
        IS_SERIALIZABLE
    };

    struct RADAPTER_API RedisCacheConsumer : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Optional<QString>, object_hash_key)
        FIELD(HasDefault<quint32>, update_rate, 600)
    };

    struct RADAPTER_API RedisCacheProducer : RedisConnector {
        Q_GADGET
        IS_SERIALIZABLE
        FIELD(Optional<QString>, object_hash_key)
    };
}

#endif // REDISSETTINGS_H
