#ifndef SQLKEYVAULTFIELDS_H
#define SQLKEYVAULTFIELDS_H
#include <QLatin1String>
namespace Sql {
    struct KeyVaultFields {
        static const constexpr char * redisKey{"redis_key"};
        static const constexpr char * sourceType{"source_type_id"};
        static const constexpr char * source{"source_id", };
        static const constexpr char * param{"param_id"};
        static const constexpr char * isEvent{"is_event_time"};
    };
}


#endif // SQLKEYVAULTFIELDS_H
