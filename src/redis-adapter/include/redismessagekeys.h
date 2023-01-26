#ifndef REDISMESSAGEKEYS_H
#define REDISMESSAGEKEYS_H

#define REDIS_MESSAGE_TYPE_PATTERN  "pmessage"
#define REDIS_MESSAGE_INDEX_TYPE    0

#define REDIS_PATTERN_KEY_EVENT     "__keyevent*"
#define REDIS_SEP                   ":"
#define REDIS_KEY_EVENT_TYPE_STRING "set"

#define REDIS_PMESSAGE_INDEX_PATTERN    1
#define REDIS_PMESSAGE_INDEX_CHANNEL    2
#define REDIS_PMESSAGE_INDEX_PAYLOAD    3
#define REDIS_PMESSAGE_SIZE             4

#define REDIS_SET_PREFIX QStringLiteral("__sref__")
#define REDIS_HASH_PREFIX QStringLiteral("__href__")
#define REDIS_STR_PREFIX QStringLiteral("__ref__")

#endif // REDISMESSAGEKEYS_H
