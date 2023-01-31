#include "redissettings.h"

using namespace Settings;

RedisServer::Map RedisServer::cacheMap = RedisServer::Map{};
RedisStream::Map RedisStream::cacheMap = RedisStream::Map{};
RedisCache::Map RedisCache::cacheMap = RedisCache::Map{};
RedisKeyEventSubscriber::Map RedisKeyEventSubscriber::cacheMap = RedisKeyEventSubscriber::Map{};
QMap<QString, Settings::RedisStreamConsumer::StartMode> RedisStreamConsumer::startModesMap{
    {"persistent_id", StartPersistentId},
    {"top", StartFromTop},
    {"first", StartFromFirst}
};
QMap<QString, RedisCache::Mode> RedisCache::modeMap{
    {"consumer", Consumer},
    {"producer", Producer}
};
