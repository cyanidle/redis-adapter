#include "redissettings.h"

using namespace Settings;

RedisServer::Map RedisServer::cacheMap = RedisServer::Map{};
RedisStream::Map RedisStream::cacheMap = RedisStream::Map{};
RedisCache::Map RedisCache::cacheMap = RedisCache::Map{};
RedisKeyEventSubscriber::Map RedisKeyEventSubscriber::cacheMap = RedisKeyEventSubscriber::Map{};

