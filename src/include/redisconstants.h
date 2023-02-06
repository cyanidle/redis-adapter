#ifndef REDIS_CONSTANTS_H
#define REDIS_CONSTANTS_H
#include <QLatin1String>
namespace Redis {
    struct Constants {
        static const constexpr QChar separator{':'};
        static const constexpr char * keyEvent{"__keyevent*"};
        static const int patternIndex{1};
        static const int channelIndex{2};
        static const int payloadIndex{3};
        static const int sizeIndex{4};
        static const constexpr char * set{"__sref__"};
        static const constexpr char * str{"__ref__"};
        static const constexpr char * hash{"__href__"};
    };
}

#endif // REDISMESSAGEKEYS_H
