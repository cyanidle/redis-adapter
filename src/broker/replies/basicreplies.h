#ifndef RADAPTER_BASICREPLIES_H
#define RADAPTER_BASICREPLIES_H

#include <QSet>
#include "jsondict/jsondict.hpp"
#include "private/reply.h"

namespace Radapter {

class RADAPTER_API ReplyWithReason : public Reply
{
    Q_GADGET
public:
    ReplyWithReason(bool ok, const QString &reason);
    const QString &reason() const {return m_reason;}
    RADAPTER_REPLY
protected:
    ReplyWithReason(quint32 type, bool ok, const QString &reason);
private:
    QString m_reason;
};

class RADAPTER_API ReplyOk : public Reply
{
    Q_GADGET
public:
    ReplyOk();
    RADAPTER_REPLY
};

class RADAPTER_API ReplyFail : public ReplyWithReason
{
    Q_GADGET
public:
    ReplyFail(const QString &reason = {});
    RADAPTER_REPLY
};

class RADAPTER_API ReplyJson : public Reply
{
    Q_GADGET
public:
    ReplyJson(const JsonDict &json);
    ReplyJson(JsonDict &&json);
    const JsonDict &json() const;
    RADAPTER_REPLY
protected:
    ReplyJson(quint32 type, const JsonDict &json);
private:
    JsonDict m_json;
};

class RADAPTER_API ReplyPack : public Reply
{
    Q_GADGET
public:
    ReplyPack();
    ReplyPack(std::initializer_list<Reply*> replies);
    ReplyPack(const QList<QSharedPointer<Reply>> &replies);
    ReplyPack(QList<QSharedPointer<Reply>> &&replies);
    int size() const;
    bool isEmpty() const;
    void append(QSharedPointer<Reply> reply);
    void append(Reply* reply);
    const QList<QSharedPointer<Reply>> &replies() const {return m_replies;}
    RADAPTER_REPLY
private:
    void checkPack() const;
    QList<QSharedPointer<Reply>> m_replies{};
};

} // namespace Radapter
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyWithReason, Reply::WithReason)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyOk, Reply::Ok)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyFail, Reply::Fail)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyJson, Reply::Json)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyPack, Reply::Pack)
#endif // RADAPTER_BASICREPLIES_H
