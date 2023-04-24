#include "settings.h"

bool Validator::ByteOrder::validate(QVariant &src, const QVariantList &args, QVariant &state) {
    Q_UNUSED(args)
    Q_UNUSED(state)
    typedef QMap<QString, QDataStream::ByteOrder> Map;
    static Map map{
        {"little",  QDataStream::LittleEndian},
        {"big",  QDataStream::BigEndian},
        {"littleendian",  QDataStream::LittleEndian},
        {"bigendian", QDataStream::BigEndian}
    };
    auto asStr = src.toString().toLower();
    src.setValue(map.value(asStr));
    return map.contains(asStr);
}

bool Validator::TimeZone::validate(QVariant &target, const QVariantList &args, QVariant &state) {
    Q_UNUSED(args)
    Q_UNUSED(state)
    auto time_zone = QTimeZone(target.toString().toStdString().c_str());
    target.setValue(time_zone);
    return time_zone.isValid();
}

using namespace Settings;

Q_GLOBAL_STATIC(QStringMap<TcpDevice>, tcpMap)

void TcpDevice::postUpdate() {
    if (!name->isEmpty()) {
        tcpMap->insert(name, *this);
    }
}

Q_GLOBAL_STATIC(QStringMap<SerialDevice>, serialMap)

void SerialDevice::postUpdate() {
    if (!name->isEmpty()) {
        serialMap->insert(name, *this);
    }
}

Q_GLOBAL_STATIC(QStringMap<SqlClientInfo>, sqlClientsMap)

void SqlClientInfo::postUpdate() {
    sqlClientsMap->insert(name, *this);
}
const SqlClientInfo &SqlClientInfo::get(const QString& name) {
    if (!sqlClientsMap->contains(name)) throw std::invalid_argument("Missing Sql Client with name: " + name.toStdString());
    return (*sqlClientsMap)[name];
}

void WebsocketServer::postUpdate()  {
    if (heartbeat_ms >= keepalive_time) {
        throw std::runtime_error("Cannot have 'heartbeat_ms' bigger than 'keepalive_time'");
    }
}
