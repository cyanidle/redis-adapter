#include "basicreplies.h"
#include "templates/algorithms.hpp"

namespace Radapter {

ReplyWithReason::ReplyWithReason(bool ok, const QString &reason) :
    Reply(ReplyType<ReplyWithReason>(), ok),
    m_reason(reason)
{

}

ReplyWithReason::ReplyWithReason(quint32 type, bool ok, const QString &reason) :
    Reply(type, ok),
    m_reason(reason)
{

}

ReplyOk::ReplyOk() :
    Reply(ReplyType<ReplyOk>(), true)
{

}

ReplyFail::ReplyFail(const QString &reason) :
    ReplyWithReason(ReplyType<ReplyFail>(), false, reason)
{

}

ReplyWithJson::ReplyWithJson(bool ok) :
    Reply(ReplyType<ReplyWithJson>(), ok)
{

}

ReplyWithJson::ReplyWithJson(quint32 type, bool ok) :
    Reply(type, ok)
{

}

ReplyWithJsonFail::ReplyWithJsonFail() :
    ReplyWithJson(ReplyType<ReplyWithJsonFail>(), false)
{

}

ReplyWithJsonOk::ReplyWithJsonOk() :
    ReplyWithJson(ReplyType<ReplyWithJsonOk>(), true)
{

}

ReplyPack::ReplyPack() :
    Reply(typeInConstructor(this), false)
{
}

ReplyPack::ReplyPack(std::initializer_list<Reply *> replies) :
    Reply(typeInConstructor(this), false)
{
    for (auto &reply : replies) {
        m_replies.append(QSharedPointer<Reply>(reply));
    }
    setOk(all_of(m_replies, &Reply::ok));
    checkPack();
}

ReplyPack::ReplyPack(const QList<QSharedPointer<Reply>> &replies) :
    Reply(ReplyType<ReplyPack>(), all_of(replies, &Reply::ok)),
    m_replies(replies)
{
    checkPack();
}

ReplyPack::ReplyPack(QList<QSharedPointer<Reply>> &&replies) :
    Reply(ReplyType<ReplyPack>(), all_of(replies, &Reply::ok)),
    m_replies(std::move(replies))
{
    checkPack();
}

int ReplyPack::size() const
{
    return m_replies.size();
}

bool ReplyPack::isEmpty() const
{
    return m_replies.isEmpty();
}

void ReplyPack::append(QSharedPointer<Reply> reply)
{
    m_replies.append(reply);
    setOk(all_of(m_replies, &Reply::ok));
}

void ReplyPack::append(Reply *reply)
{
    m_replies.append(QSharedPointer<Reply>(reply));
    setOk(all_of(m_replies, &Reply::ok));
}

void ReplyPack::checkPack() const
{
    for (auto &reply : m_replies) {
        if (reply->inherits<ReplyPack>()) throw std::invalid_argument("Reply Pack cannot contain Reply Packs!");
    }
}

ReplyJson::ReplyJson(const JsonDict &json) :
    Reply(typeInConstructor(this), true),
    m_json(json)
{

}

ReplyJson::ReplyJson(JsonDict &&json) :
    Reply(typeInConstructor(this), true),
    m_json(std::move(json))
{

}

const JsonDict &ReplyJson::json() const
{
    return m_json;
}

ReplyJson::ReplyJson(quint32 type, const JsonDict &json) :
    Reply(type, true),
    m_json(json)
{

}

} // namespace Radapter
