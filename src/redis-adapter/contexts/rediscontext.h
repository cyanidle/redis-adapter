#ifndef REDIS_CONTEXT_H
#define REDIS_CONTEXT_H

#include "context/asynccontext.h"
#include "jsondict/jsondict.hpp"
#include "radapter-broker/basiccommands.h"
#include "radapter-broker/workermsg.h"
#include <QObject>

namespace Redis {
class Connector;
class CacheContext : public Radapter::AsyncContext
{
    Q_OBJECT
public:
    CacheContext();
private:
    bool m_recursive{false};
    Radapter::CommandPack m_commands{};
    Radapter::ReplyPack m_replies{};
    Radapter::WorkerMsg m_msg;
};
} // namespace Redis

#endif // REDIS_CONTEXT_H
