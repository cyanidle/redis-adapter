#ifndef RADAPTERCONFIG_H
#define RADAPTERCONFIG_H

#include "broker/workers/loggingworkersettings.h"
#include "broker/brokersettings.h"
#include "broker/workers/mockworkersettings.h"
#include "interceptors/duplicatinginterseptor_settings.h"
#include "interceptors/validatinginterceptor_settings.h"
#include "raw_sockets/udpconsumer.h"
#include "raw_sockets/udpproducer.h"
#include "settings-parsing/serializablesettings.h"
#include "settings/modbussettings.h"
#include "settings/redissettings.h"
#include "settings/settings.h"

namespace Settings {

struct Udp : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<::Udp::ProducerSettings>, producers)
    FIELD(OptionalSequence<::Udp::ConsumerSettings>, consumers)
};

struct Sockets : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Optional<Udp>, udp)
};

struct Interceptors : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalMapping<ValidatingInterceptor>, validating)
    FIELD(OptionalMapping<DuplicatingInterceptor>, duplicating)

};

struct Stream : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<RedisStreamConsumer>, consumers)
    FIELD(OptionalSequence<RedisStreamProducer>, producers)
};
struct Cache : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<RedisCacheConsumer>, consumers)
    FIELD(OptionalSequence<RedisCacheProducer>, producers)
};
struct KeyEvents : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<RedisKeyEventSubscriber>, subscribers)
};

struct Redis : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(RequiredSequence<RedisServer>, servers)
    FIELD(Optional<Stream>, stream)
    FIELD(Optional<Cache>, cache)
    FIELD(Optional<KeyEvents>, key_events)
};

struct Modbus : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(RequiredSequence<ModbusDevice>, devices)
    FIELD(Required<QVariantMap>, registers)
    FIELD(OptionalSequence<ModbusSlave>, slaves)
    FIELD(OptionalSequence<ModbusMaster>, masters)
    POST_UPDATE {
        Settings::parseRegisters(registers);
        for (auto &slave: slaves) {
            slave.init();
        }
        for (auto &master: masters) {
            master.init();
        }
    }
};

struct Websocket : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<WebsocketClient>, clients)
    FIELD(OptionalSequence<WebsocketServer>, servers)
};

struct Sql : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<SqlClientInfo>, clients)
    FIELD(OptionalSequence<SqlStorageInfo>, archives)
};

struct AppConfig : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Optional<QVariantMap>, json_routes)
    FIELD(Optional<QVariantMap>, filters) //not implemented
    FIELD(Optional<QVariantMap>, log_debug) //not implemented
    FIELD(Optional<Interceptors>, interceptors)
    FIELD(Optional<Broker>, broker)
    FIELD(Optional<LocalizationInfo>, localization)
    FIELD(OptionalSequence<LoggingWorker>, logging_workers)
    FIELD(OptionalSequence<MockWorker>, mocks)
    FIELD(Optional<Redis>, redis)
    FIELD(Optional<Modbus>, modbus)
    FIELD(Optional<Sockets>, sockets)
    FIELD(Optional<Websocket>, websocket)
    FIELD(OptionalSequence<QString>, pipelines)
    void postUpdate() override;
};
}

#endif // RADAPTERCONFIG_H
