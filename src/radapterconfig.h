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
    FIELD(NonRequiredSequence<::Udp::ProducerSettings>, producers)
    FIELD(NonRequiredSequence<::Udp::ConsumerSettings>, consumers)
};

struct Sockets : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequired<Udp>, udp)
};

struct Interceptors : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredMapping<ValidatingInterceptor>, validating)
    FIELD(NonRequiredMapping<DuplicatingInterceptor>, duplicating)

};

struct Stream : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredSequence<RedisStreamConsumer>, consumers)
    FIELD(NonRequiredSequence<RedisStreamProducer>, producers)
};
struct Cache : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredSequence<RedisCacheConsumer>, consumers)
    FIELD(NonRequiredSequence<RedisCacheProducer>, producers)
};
struct KeyEvents : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredSequence<RedisKeyEventSubscriber>, subscribers)
};

struct Redis : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredSequence<RedisServer>, servers)
    FIELD(NonRequired<Stream>, stream)
    FIELD(NonRequired<Cache>, cache)
    FIELD(NonRequired<KeyEvents>, key_events)
};

struct Modbus : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredSequence<ModbusDevice>, devices)
    FIELD(NonRequiredSequence<ModbusSlave>, slaves)
    FIELD(NonRequiredSequence<ModbusMaster>, masters)
    FIELD(NonRequired<QVariantMap>, registers)
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
    FIELD(NonRequiredSequence<WebsocketClient>, clients)
    FIELD(NonRequiredSequence<WebsocketServer>, servers)
};

struct Sql : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequiredSequence<SqlClientInfo>, clients)
    FIELD(NonRequiredSequence<SqlStorageInfo>, archives)
};

struct AppConfig : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(NonRequired<QVariantMap>, json_routes)
    FIELD(NonRequired<QVariantMap>, filters) //not implemented
    FIELD(NonRequired<QVariantMap>, log_debug) //not implemented
    FIELD(NonRequired<Interceptors>, interceptors)
    FIELD(NonRequired<Broker>, broker)
    FIELD(NonRequired<LocalizationInfo>, localization)
    FIELD(NonRequiredSequence<LoggingWorker>, logging_workers)
    FIELD(NonRequiredSequence<MockWorker>, mocks)
    FIELD(NonRequired<Redis>, redis)
    FIELD(NonRequired<Modbus>, modbus)
    FIELD(NonRequired<Sockets>, sockets)
    FIELD(NonRequiredSequence<QString>, pipelines)

};
}

#endif // RADAPTERCONFIG_H
