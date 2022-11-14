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

    struct RADAPTER_SHARED_SRC WorkerSettings : public Serializer::SerializerBase {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name, "None");
        SERIAL_FIELD(bool, debug, false)
        SERIAL_CONTAINER(QList, QString, producers, QStringList())
        SERIAL_CONTAINER(QList, QString, consumers, QStringList())
        SERIAL_FIELD(quint32, max_msgs_in_queue, 30)

        Radapter::WorkerSettings asWorkerSettings(QThread *thread) const {
            return Radapter::WorkerSettings(
                name,
                thread,
                consumers,
                producers,
                debug,
                max_msgs_in_queue
                );
        }
    };


    struct RADAPTER_SHARED_SRC RecordOutgoingSetting : public Serializer::SerializerBase {
        using targetSetting = Radapter::LoggingInterceptorSettings;
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(bool, use, false)
        SERIAL_FIELD(QString, filepath, "")
        SERIAL_CUSTOM(targetSetting::LogMsg,
                      log, initLog, CUSTOM_NO_READ,
                      targetSetting::LogNormal)
        SERIAL_FIELD(quint32, flushDelay, 1000)
        SERIAL_FIELD(quint64, maxSizeBytes, 100000000)
        SERIAL_FIELD(quint16, maxFiles, 10)
        SERIAL_CUSTOM(QJsonDocument::JsonFormat, format, initFormat, CUSTOM_NO_READ, QJsonDocument::Indented)

        bool initFormat(const QVariant &src) {
            auto strRep = src.toString().toLower();
            if (strRep == "compact") {
                format = QJsonDocument::Compact;
            } else {
                format = QJsonDocument::Indented;
            }
            return true;
        }
        bool initLog(const QVariant &src) {
            const auto strRep = src.toString().toLower();
            if (strRep == "all") {
                log = targetSetting::LogAll;
            } else {
                const auto splitVersions = Serializer::convertQList<QString>(src.toList());
                if (splitVersions.contains("normal") || strRep == "normal") {
                    log |= targetSetting::LogNormal;
                }
                if (splitVersions.contains("reply") || strRep == "reply") {
                    log |= targetSetting::LogReply;
                }
                if (splitVersions.contains("command") || strRep == "command") {
                    log |= targetSetting::LogCommand;
                }
            }
            return true;
        }
        Radapter::LoggingInterceptorSettings asSettings() const {
            Radapter::LoggingInterceptorSettings result;
            result.filePath = filepath;
            result.flushTimerDelay = flushDelay;
            result.format = format;
            result.maxFileSizeBytes = maxSizeBytes;
            result.maxFiles = maxFiles;
            result.logFlags = log;
            return result;
        }
    };

    struct RADAPTER_SHARED_SRC MockWorkerSettings : public WorkerSettings {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(quint32, mock_timer_delay, 1000)
        SERIAL_FIELD(QString, json_file_path)
        Radapter::MockWorkerSettings asMockSettings(QThread *thread) const {
            Radapter::MockWorkerSettings result;
            result.mockTimerDelay = mock_timer_delay;
            result.jsonFilePath = json_file_path;
            result.name = name;
            result.thread = thread;
            result.consumers = consumers;
            result.producers = producers;
            result.isDebug = debug;
            result.maxMsgsInQueue = max_msgs_in_queue;
            return result;
        }
    };
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
        SERIAL_FIELD(QString, name);
        SERIAL_FIELD(QString, ip);
        SERIAL_FIELD(quint16, port)
        SERIAL_POST_INIT(cache)
        void cache() {
            cacheMap.insert(name, *this);
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
        SERIAL_FIELD(QString, name)
        SERIAL_FIELD(QString, client_name)
        SERIAL_FIELD(QString, target_table)
        SERIAL_FIELD(QString, key_vault_name)
        SqlKeyVaultInfo key_vault;
        SERIAL_CONTAINER(QList, QString, producers)
        SERIAL_POST_INIT(postInit)

        void postInit()
        {
            key_vault = SqlKeyVaultInfo::cacheMap.value(key_vault_name);
        }

        bool isValid() const {
            return !client_name.isEmpty() && !target_table.isEmpty()
                    && key_vault.isValid() && !producers.isEmpty();
        }
        bool operator==(const SqlStorageInfo &other) const {
            return this->client_name == other.client_name
                    && this->target_table == other.target_table
                    && this->key_vault == other.key_vault
                    && this->producers == other.producers;
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
        SERIAL_CONTAINER(QList, QString, producers)
        SERIAL_CONTAINER(QList, QString, consumers)

        bool isValid() const {
            return (port > 0u)
                    && !(producers.isEmpty() && consumers.isEmpty());
        }
        bool operator==(const WebsocketServerInfo &other) const {
            return (port == other.port)
                    && (producers == other.producers)
                    && (consumers == other.consumers);
        }
        bool operator!=(const WebsocketServerInfo &other) const {
            return !(*this == other);
        }
    };

    struct RADAPTER_SHARED_SRC WebsockerClientInfo : WorkerSettings {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, server_host, "localhost");
        SERIAL_FIELD(quint16, server_port, 0);
    };
}

#endif // SETTINGS_H
