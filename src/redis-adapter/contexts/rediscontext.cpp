#include "rediscontext.h"

namespace Redis {

Context::Context(QObject *parent)
    : Radapter::AsyncContext{parent}
{

}

} // namespace Redis
