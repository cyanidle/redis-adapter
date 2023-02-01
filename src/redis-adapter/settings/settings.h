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
#include "radapter-broker/workerbasesettings.h"
#include <settings-parsing/serializer.hpp>
#include <QJsonDocument>

namespace Settings {

    struct RADAPTER_SHARED_SRC ServerInfo : public Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, host)
        SERIAL_FIELD(quint16, port)
    };

    struct RADAPTER_SHARED_SRC TcpDevice : public ServerInfo {
        typedef QMap<QString, TcpDevice> Map;
        static Map &cacheMap() {
            static Map map{};
            return map;
        }
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        QString repr() const {
            return QStringLiteral("Tcp dev %1; Host: %2; Port: %3")
                .arg(name, host)
                .arg(port);
        }
        SERIAL_POST_INIT(cache)
        void cache() {
            cacheMap().insert(name, *this);
        }
    };
    
    struct RADAPTER_SHARED_SRC SqlClientInfo : ServerInfo {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, name)
        SERIAL_FIELD(QString, database)
        SERIAL_FIELD(QString, username)
        SERIAL_POST_INIT([this](){table().insert(name, *this);})
        typedef QMap<QString, SqlClientInfo> Map;
        static Map& table() {
            static Map map{};
            return map;
        }
    };

    struct RADAPTER_SHARED_SRC SqlStorageInfo : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        SERIAL_FIELD(QString, name)
        SERIAL_FIELD(QString, client_name)
        SERIAL_FIELD(QString, target_table)
        SERIAL_FIELD(QString, table_name);
    };

    typedef QMap<QString /*module*/, bool /*mode*/> LoggingInfo;
    struct RADAPTER_SHARED_SRC LoggingInfoParser{
        static LoggingInfo parse(const QVariantMap &src) {
            return Serializer::convertQMap<bool>(src);
        }
    };

    struct RADAPTER_SHARED_SRC Filters {
        typedef QMap<QString /*filterName*/, double> Table;
        typedef QMap<QString /*filterName*/, Table> TableMap;
        static TableMap &table() {
            static TableMap map{};
            return map;
        }
    };

    struct RADAPTER_SHARED_SRC LocalizationInfo : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_CUSTOM(QTimeZone, time_zone, initTimeZone, readTimeZone, QTimeZone::systemTimeZone());
        bool initTimeZone(const QVariant &src) {
            time_zone = QTimeZone(src.toString().toStdString().c_str());
            return time_zone.isValid();
        }
        QVariant readTimeZone() const {
            return time_zone.displayName(QTimeZone::TimeType::GenericTime, QTimeZone::NameType::DefaultName);
        }
        bool isValid() const {
            return time_zone.isValid();
        }
    };

    struct RADAPTER_SHARED_SRC WebsocketServerInfo : Serializer::SerializableGadget {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(quint16, port, 1234)
        SERIAL_FIELD(Radapter::WorkerSettings, worker)
        bool isValid() const {
            return worker.isValid();
        }
    };

    struct RADAPTER_SHARED_SRC WebsocketClientInfo : WebsocketServerInfo {
        Q_GADGET
        IS_SERIALIZABLE
        SERIAL_FIELD(QString, host)
    };
}

#endif // SETTINGS_H
