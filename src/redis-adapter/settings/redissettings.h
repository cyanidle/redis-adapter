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
        bool isValid() {
            return ServerInfo::isValid() && !name.isEmpty();
        }
        bool operator==(const RedisServer &src) const {
           return name == src.name
               && server_host == src.server_host
               && server_port == src.server_port;
        }
        bool operator!=(const RedisServer &src)const {
            return !(*this == src);
        }
    };

    struct RADAPTER_SHARED_SRC RedisConnector : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(quint16, db_index, 0)
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, target_server_name)

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

        bool isValid() {
            return !worker.name.isEmpty() && !source_server_name.isEmpty()
                    && source_server.isValid() && !keyEvents.isEmpty();
        }

        bool operator==(const RedisKeyEventSubscriber &other) const {
            return worker.name == other.worker.name
                    && source_server_name == other.source_server_name
                    && source_server == other.source_server
                    && keyEvents == other.keyEvents;
        }

        bool operator!=(const RedisKeyEventSubscriber &other) const {
            return !(*this == other);
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

        bool isValid() {
            if (worker.name.isEmpty() || stream_key.isEmpty()) {
                return false;
            }
            bool isValid = false;
            if (mode == RedisStreamConsumer) {
                isValid = !source_server.isEmpty() && source.isValid();
            } else if (mode == RedisStreamProducer) {
                isValid = !target_server.isEmpty() && target.isValid() && (stream_size != 0);
            }
            return isValid;
        }
        bool operator==(const RedisStream &src)const {
           return worker.name == src.worker.name
               && mode == src.mode
               && target_server == src.target_server
               && target == src.target
               && source_server == src.source_server
               && source == src.source
               && stream_key == src.stream_key
               && stream_size == src.stream_size;
        }
        bool operator!=(const RedisStream &src)const {
            return !(*this == src);
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
        Q_GADGET
        IS_SERIALIZABLE
        RedisServer target_server;
        SERIAL_FIELD_PTR(Radapter::LoggingInterceptorSettings, log_jsons, DEFAULT)
        SERIAL_FIELD(QString, index_key)
        SERIAL_CUSTOM(Mode, mode, initMode, NO_READ)
        SERIAL_POST_INIT(postInit)

        static inline QList<RedisCache> getCaches(const QVariant &src) {
            return Serializer::fromQList<RedisCache>(src.toList());
        }

        bool initMode(const QVariant &src) {
            auto srcStr = src.toString();
            if (srcStr == "consumer") {
                mode = Consumer;
                return true;
            } else if (srcStr == "producer") {
                mode = Producer;
                return true;
            }
            return false;
        }

        void postInit() {
            target_server = RedisServer::cacheMap.value(target_server_name);
            cacheMap.insert(worker.name, *this);
        }

        bool isValid() {
            return !worker.name.isEmpty() && !target_server_name.isEmpty()
                    && target_server.isValid() && !index_key.isEmpty();
        }

        bool operator==(const RedisCache &src)const {
            return worker.name == src.worker.name
                && target_server_name == src.target_server_name
                && target_server == src.target_server
                && index_key == src.index_key
                && worker.producers == src.worker.producers
                && worker.consumers == src.worker.consumers;
        }
        bool operator!=(const RedisCache &src)const {
            return !(*this == src);
        }
    };

}

#endif // REDISSETTINGS_H
