#ifndef RADAPTER_BASICREPLIES_H
#define RADAPTER_BASICREPLIES_H

#include <QSet>
#include "jsondict/jsondict.hpp"
#include "reply.h"

namespace Radapter {

class RADAPTER_SHARED_SRC ReplyWithReason : public Reply
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

class RADAPTER_SHARED_SRC ReplyOk : public Reply
{
    Q_GADGET
public:
    ReplyOk();
    RADAPTER_REPLY
};

class RADAPTER_SHARED_SRC ReplyFail : public ReplyWithReason
{
    Q_GADGET
public:
    ReplyFail(const QString &reason = {});
    RADAPTER_REPLY
};

class RADAPTER_SHARED_SRC ReplyJson : public Reply
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

//! Means msg contains? Json Reply in body
class RADAPTER_SHARED_SRC ReplyWithJson : public Reply
{
    Q_GADGET
public:
    ReplyWithJson(bool ok);
    RADAPTER_REPLY
protected:
    ReplyWithJson(quint32 type, bool ok);
};


//! Means msg contains Json Reply in body
class RADAPTER_SHARED_SRC ReplyWithJsonOk : public ReplyWithJson
{
    Q_GADGET
public:
    ReplyWithJsonOk();
    RADAPTER_REPLY
};

//! Means msg does not contain Json Reply in body (failed)
class RADAPTER_SHARED_SRC ReplyWithJsonFail : public ReplyWithJson
{
    Q_GADGET
public:
    ReplyWithJsonFail();
    RADAPTER_REPLY
};

class RADAPTER_SHARED_SRC ReplyPack : public Reply
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
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyWithJson, Reply::WithJson)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyJson, Reply::Json)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyWithJsonOk, Reply::WithJsonOk)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyWithJsonFail, Reply::WithJsonFail)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::ReplyPack, Reply::Pack)
#endif // RADAPTER_BASICREPLIES_H
