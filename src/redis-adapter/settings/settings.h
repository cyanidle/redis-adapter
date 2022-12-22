#ifndef SETTINGS_H
#define SETTINGS_H

#include <QObject>
#include <QSerialPort>
#include <QVariant>
#include <QDataStream>
#include <QVector>
#include <QTime>
#include <QModbusDataUnit>
#include <QTimeZone>
#include "qthread.h"
#include "redis-adapter/radapterlogging.h"
#include "radapter-broker/workerbasesettings.h"
#include "radapter-broker/debugging/mockworkersettings.h"
#include "radapter-broker/debugging/logginginterceptorsettings.h"
#include <settings-parsing/serializerbase.h>
#include <QJsonDocument>

namespace Settings {

    struct RADAPTER_SHARED_SRC ServerInfo : public Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, server_host);
        SERIAL_FIELD(quint16, server_port);
        bool isValid() {
            return !server_host.isEmpty() && (server_port > 0u);
        }
        bool operator==(const ServerInfo &src) const {
           return server_host == src.server_host
                && server_port == src.server_port;
        }
        bool operator!=(const ServerInfo &src) const {
            return !(*this == src);
        }
    };

    struct RADAPTER_SHARED_SRC TcpDevice : public Serializer::SerializerBase {
        typedef QMap<QString, TcpDevice> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name, "");
        SERIAL_FIELD(QString, ip);
        SERIAL_FIELD(quint16, port)
        QString repr() const {
            return QStringLiteral("Tcp dev %1; Ip: %2; Port: %3")
                .arg(name, ip)
                .arg(port);
        }
        SERIAL_POST_INIT(cache)
        void cache() {
            if (!name.isEmpty()) {
                cacheMap.insert(name, *this);
            }
        }
        bool isValid() {
            return !ip.isEmpty() && (port > 0u);
        }
        bool operator==(const TcpDevice src) const {
            return ip == src.ip
                && port == src.port;
        }
        bool operator!=(const TcpDevice &src) const {
            return !(*this == src);
        }
    };


    struct RADAPTER_SHARED_SRC Filters  : Serializer::SerializerBase {
        typedef QMap<QString /*filterName*/, double> Table;
        typedef QMap<QString /*filterName*/, Table> TableMap;
        static TableMap tableMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        SERIAL_MAP(double, fields)
        SERIAL_POST_INIT(cache)
        void cache() {
            tableMap.insert(name, fields);
        }
    };


    
    struct RADAPTER_SHARED_SRC SqlClientInfo  : Serializer::SerializerBase {
        typedef QMap<QString, SqlClientInfo> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        SERIAL_FIELD(QString, ip)
        SERIAL_FIELD(bool, debug, false)
        SERIAL_FIELD(quint16, port)
        SERIAL_FIELD(QString, database)
        SERIAL_FIELD(QString, username)

        SERIAL_POST_INIT(cache)
        void cache() {
            cacheMap.insert(name, *this);
        }

        bool isValid() {
            return !name.isEmpty() && !ip.isEmpty()
                    && port != 0u && !database.isEmpty()
                    && !username.isEmpty();
        }
        bool operator==(const SqlClientInfo &src) const{
            return name == src.name
                && ip == src.ip
                && port == src.port
                && database == src.database
                && username == src.username;
        }

        bool operator!=(const SqlClientInfo &other) {
            return !(*this == other);
        }
    };

    struct RADAPTER_SHARED_SRC SqlKeyVaultInfo : Serializer::SerializerBase {
        typedef QMap<QString, SqlKeyVaultInfo> Map;
        static Map cacheMap;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        SERIAL_FIELD(QString, table_name)

        bool isValid() const {
            return !name.isEmpty() && !table_name.isEmpty();
        }
        bool operator==(const SqlKeyVaultInfo &other) const {
            return this->name == other.name
                    && this->table_name == other.table_name;
        }
        bool operator!=(const SqlKeyVaultInfo &other) const {
            return !(*this == other);
        }
    };

    struct RADAPTER_SHARED_SRC SqlStorageInfo : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_NEST(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, name)
        SERIAL_FIELD(QString, client_name)
        SERIAL_FIELD(QString, target_table)
        SERIAL_FIELD(QString, key_vault_name)
        SqlKeyVaultInfo key_vault;
        SERIAL_POST_INIT(postInit)

        void postInit()
        {
            key_vault = SqlKeyVaultInfo::cacheMap.value(key_vault_name);
        }

        bool isValid() const {
            return !client_name.isEmpty() && !target_table.isEmpty()
                    && key_vault.isValid();
        }
        bool operator==(const SqlStorageInfo &other) const {
            return this->client_name == other.client_name
                    && this->target_table == other.target_table
                    && this->key_vault == other.key_vault;
        }
        bool operator!=(const SqlStorageInfo &other) const {
            return !(*this == other);
        }
    };

    typedef QMap<QString /*module*/, bool /*mode*/> LoggingInfo;
    struct RADAPTER_SHARED_SRC LoggingInfoParser{
        static LoggingInfo parse(const QVariantMap &src);
    };

    struct RADAPTER_SHARED_SRC LocalizationInfo : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CUSTOM(QTimeZone, time_zone, initTimeZone, readTimeZone);

        bool initTimeZone(const QVariant &src) {
            auto rawTime = src.toString();
            time_zone = QTimeZone(rawTime.toStdString().c_str());
            return time_zone.isValid();
        }

        QVariant readTimeZone() const {
            return time_zone.displayName(QTimeZone::TimeType::GenericTime, QTimeZone::NameType::DefaultName);
        }

        bool isValid() const {
            return time_zone.isValid();
        }

        bool operator==(const LocalizationInfo &other) const {
            return this->time_zone == other.time_zone;
        }
        bool operator!=(const LocalizationInfo &other) const {
            return !(*this == other);
        }
    };

    struct RADAPTER_SHARED_SRC WebsocketServerInfo : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(quint16, port)
        SERIAL_FIELD(bool, debug, false)
        SERIAL_NEST(Radapter::WorkerSettings, worker)

        bool isValid() const {
            return (port > 0u)
                    && !(worker.producers.isEmpty() && worker.consumers.isEmpty());
        }
        bool operator==(const WebsocketServerInfo &other) const {
            return (port == other.port)
                    && (worker.producers == other.worker.producers)
                    && (worker.consumers == other.worker.consumers);
        }
        bool operator!=(const WebsocketServerInfo &other) const {
            return !(*this == other);
        }
    };

    struct RADAPTER_SHARED_SRC WebsocketClientInfo : Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_NEST(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, server_host, "localhost");
        SERIAL_FIELD(quint16, server_port, 0);
    };
}

#endif // SETTINGS_H
