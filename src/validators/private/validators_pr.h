#ifndef VALIDATORS_PRIVATE_H
#define VALIDATORS_PRIVATE_H

#include "private/global.h"

namespace Validator {
using Function = bool (*)(QVariant &target, const QVariantList &args, QVariant& state);
namespace Private {
int add(Function func, const QStringList &aliases);
int add(Function func, const char **aliases, int count);
int add(Function func, const char *alias);
template <typename...Names>
int add(Function func, const char *alias, const char *alias2, Names...aliases) {
    return add(func, alias) + add(func, alias2, aliases...);
}
}
}
Q_DECLARE_METATYPE(Validator::Function)
#define VALIDATOR_DECLARE_ALIASES(...) \
static const char **_aliases() { \
        static const char *_aliases_impl[]{__VA_ARGS__};\
        return _aliases_impl;\
} \
    static int _aliases_count() { \
        const char *_aliases_impl[]{__VA_ARGS__};\
        return sizeof(_aliases_impl)/sizeof(const char *); \
}
#endif
