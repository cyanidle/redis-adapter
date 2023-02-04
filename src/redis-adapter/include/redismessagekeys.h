#ifndef REDISMESSAGEKEYS_H
#define REDISMESSAGEKEYS_H

#define REDIS_MESSAGE_TYPE_PATTERN  QStringLiteral("pmessage")
#define REDIS_MESSAGE_INDEX_TYPE    0

#define REDIS_PATTERN_KEY_EVENT     QStringLiteral("__keyevent*")
#define REDIS_SEP                   QStringLiteral(":")
#define REDIS_KEY_EVENT_TYPE_STRING QStringLiteral("set")

#define REDIS_PMESSAGE_INDEX_PATTERN    1
#define REDIS_PMESSAGE_INDEX_CHANNEL    2
#define REDIS_PMESSAGE_INDEX_PAYLOAD    3
#define REDIS_PMESSAGE_SIZE             4

#define REDIS_SET_PREFIX QStringLiteral("__sref__:")
#define REDIS_HASH_PREFIX QStringLiteral("__href__:")
#define REDIS_STR_PREFIX QStringLiteral("__ref__:")

#endif // REDISMESSAGEKEYS_H
