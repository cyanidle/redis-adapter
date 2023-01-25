#ifndef REDIS_REPLIES_H
#define REDIS_REPLIES_H

#include "radapter-broker/reply.h"

namespace Redis {

class IndexReply : public Radapter::Reply
{
    Q_GADGET
public:
    IndexReply(const QStringList &keys);
    GADGET_BODY
    const QStringList &keys() const {return m_keys;}
private:
    QStringList m_keys;
};

} // namespace Redis
RADAPTER_DECLARE_REPLY(Redis::IndexReply)
#endif // REDIS_REPLIES_H
