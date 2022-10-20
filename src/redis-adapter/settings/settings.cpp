#include "settings.h"

using namespace Settings;

TcpDevice::Map TcpDevice::cacheMap = {};
Filters::TableMap Filters::tableMap = {};
SqlClientInfo::Map SqlClientInfo::cacheMap = {};
SqlKeyVaultInfo::Map SqlKeyVaultInfo::cacheMap = {};

LoggingInfo LoggingInfoParser::parse(const QVariantMap &src) {
    auto result = Serializer::convertQMap<bool>(src);
    return result;
}
