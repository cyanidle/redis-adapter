#ifndef RADAPTERCONFIG_H
#define RADAPTERCONFIG_H

#include "broker/workers/settings/fileworkersettings.h"
#include "broker/brokersettings.h"
#include "broker/workers/settings/mockworkersettings.h"
#include "broker/workers/settings/processworkersettings.h"
#include "broker/workers/settings/repeatersettings.h"
#include "filters/producerfiltersettings.hpp"
#include "httpserver/radapterapisettings.h"
#include "interceptors/settings/duplicatinginterseptorsettings.h"
#include "interceptors/settings/namespaceunwrappersettings.h"
#include "interceptors/settings/namespacewrappersettings.h"
#include "interceptors/settings/remappingpipesettings.h"
#include "interceptors/settings/renamingpipesettings.h"
#include "interceptors/settings/validatinginterceptorsettings.h"
#include "raw_sockets/udpconsumer.h"
#include "raw_sockets/udpproducer.h"
#include "settings-parsing/serializablesetting.h"
#include "settings/modbussettings.h"
#include "settings/redissettings.h"
#include "settings/settings.h"

namespace Settings {

struct UdpSockets : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<Udp::ProducerSettings>, producers)
    FIELD(OptionalSequence<Udp::ConsumerSettings>, consumers)
};

struct Sockets : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Optional<UdpSockets>, udp)
};

struct NamespaceInterceptors : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalMapping<NamespaceUnwrapper>, unwrappers)
    FIELD(OptionalMapping<NamespaceWrapper>, wrappers)
};

struct Interceptors : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalMapping<DuplicatingInterceptor>, duplicating)
    FIELD(OptionalMapping<ValidatingInterceptor>, validating)
    FIELD(Optional<NamespaceInterceptors>, namespaces)
    FIELD(OptionalMapping<ProducerFilter>, filters)
    FIELD(OptionalMapping<RenamingPipe>, renaming)
    FIELD(OptionalMapping<RemappingPipe>, remapping)

};

struct Stream : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<RedisStreamConsumer>, consumers)
    FIELD(OptionalSequence<RedisStreamProducer>, producers)
};
struct Cache : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<RedisCacheConsumer>, consumers)
    FIELD(OptionalSequence<RedisCacheProducer>, producers)
};
struct KeyEvents : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<RedisKeyEventSubscriber>, subscribers)
};

struct Redis : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(RequiredSequence<RedisServer>, servers)
    FIELD(Optional<Stream>, stream)
    FIELD(Optional<Cache>, cache)
    FIELD(Optional<KeyEvents>, key_events)
};

struct Modbus : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Required<Registers>, registers)
    COMMENT(registers, "Registers are initialized first")
    FIELD(RequiredSequence<ModbusDevice>, devices)
    COMMENT(devices, "Then devices")
    FIELD(OptionalSequence<ModbusSlave>, slaves)
    FIELD(OptionalSequence<ModbusMaster>, masters)
};

struct Websocket : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<WebsocketClient>, clients)
    FIELD(OptionalSequence<WebsocketServer>, servers)
};

struct Sql : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalSequence<SqlClientInfo>, clients)
    FIELD(OptionalSequence<SqlStorageInfo>, archives)
};

struct AppConfig : public Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(Optional<QVariantMap>, log_debug) //not implemented
    FIELD(Optional<Interceptors>, interceptors)
    FIELD(OptionalSequence<Repeater>, repeaters)
    FIELD(Optional<Broker>, broker)
    FIELD(Optional<LocalizationInfo>, localization)
    FIELD(OptionalSequence<FileWorker>, files)
    FIELD(OptionalSequence<MockWorker>, mocks)
    FIELD(OptionalSequence<ProcessWorker>, processes)
    FIELD(Optional<Redis>, redis)
    FIELD(Optional<Modbus>, modbus)
    FIELD(Optional<Sockets>, sockets)
    FIELD(Optional<Websocket>, websocket)
    FIELD(OptionalSequence<QString>, pipelines)
    COMMENT(pipelines, "Example: 'worker.name > *interceptor > worker.2.name'")
    FIELD(HasDefault<RadapterApi>, api)
    void postUpdate() override;
};
}

#endif // RADAPTERCONFIG_H
