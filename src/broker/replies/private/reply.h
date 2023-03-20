#ifndef RADAPTER_REPLY_H
#define RADAPTER_REPLY_H

#include <QString>
#include <QSharedPointer>
#include <limits>
#include "private/commandreplymacros.h"
#include "templates/metaprogramming.hpp"

namespace Radapter {

class RADAPTER_API Reply
{
    Q_GADGET
public:
    enum Type : quint32 {
        None = 0,
        WithReason,
        Ok,
        Fail,
        Json,
        WithJson,
        WithJsonOk,
        WithJsonFail,
        Pack = 100,

        User = 500,
        MaxUser = std::numeric_limits<std::underlying_type<Type>::type>::max(),
    };
    bool ok() const {return m_ok;}
    bool isUser() const {return User < type();}
    void setOk (bool ok) {m_ok = ok;}
    Type type() const {return static_cast<Type>(m_type);}
    virtual ~Reply() = default;
    virtual Reply* newCopy() const {return new Reply(*this);}
    virtual const QMetaObject *metaObject() const {return &this->staticMetaObject;}
    template <typename Target> bool is() const;
    template <typename Target> Target *as();
    template <typename Target> const Target *as() const;
    template <typename Target> bool inherits() const;
    static quint32 generateUserType();
    void *voidCast(const QMetaObject* meta);
    const void *voidCast(const QMetaObject* meta) const;
protected:
    Reply(quint32 type, bool ok);
    template <typename Target> Type typeInConstructor(const Target* thisPtr) const;
private:
    quint32 m_type;
    bool m_ok;
};

template <typename Target>
struct ReplyInfo {
    static Reply::Type type() {
        return Reply::None;
    }
    static const QMetaObject *metaObject() {
        return nullptr;
    }
    enum {
        Defined = false,
        IsBuiltIn = false
    };
};
template <typename Target>
Reply::Type ReplyType() {
    static_assert(ReplyInfo<Target>::Defined, "Use RADAPTER_DECLARE_REPLY(YourReply)");
    return ReplyInfo<Target>::type();
}
template <typename WantedReply>
WantedReply reply_cast(const Reply *reply) {
    using decayed = typename std::remove_cv<typename std::remove_pointer<WantedReply>::type>::type;
    static_assert(ReplyInfo<decayed>::Defined, "Use RADAPTER_DECLARE_REPLY(YourReply)");
    if (reply && ReplyType<decayed>() == reply->type()) {
        return static_cast<WantedReply>(reply);
    }
    return static_cast<WantedReply>(reply->voidCast(&decayed::staticMetaObject));
}
template <typename WantedReply>
WantedReply reply_cast(Reply *reply) {
    using decayed = typename std::remove_cv<typename std::remove_pointer<WantedReply>::type>::type;
    static_assert(ReplyInfo<decayed>::Defined, "Use RADAPTER_DECLARE_REPLY(YourReply)");
    if (reply && ReplyType<decayed>() == reply->type()) {
        return static_cast<WantedReply>(reply);
    }
    return static_cast<WantedReply>(reply->voidCast(&decayed::staticMetaObject));
}

template <typename Target, Reply::Type builtin = Reply::Type::None>
struct ReplyInfoSpecialized {
    static_assert(Radapter::Gadget_With_MetaObj<Target>::Value, "Place in class body RADAPTER_REPLY");
    static Reply::Type type() {
        static quint32 type{IsBuiltIn ? builtin : Reply::generateUserType()};;
        return static_cast<Reply::Type>(type);
    }
    static const QMetaObject *metaObject() {
        return &Target::staticMetaObject;
    }
    enum {
        Defined = true,
        IsBuiltIn = builtin != Reply::None
    };
};
template <typename Target> Target *Reply::as()
{
    return reply_cast<Target *>(this);
}
template <typename Target> const Target *Reply::as() const
{
    return reply_cast<const Target *>(this);
}
template<typename Target>
bool Reply::is() const
{
    return type() == ReplyType<Target>() || inherits<Target>();
}

template<typename Target>
bool Reply::inherits() const
{
    return metaObject()->inherits(ReplyInfo<Target>::metaObject());
}

template<typename Target>
Reply::Type Reply::typeInConstructor(const Target* thisPtr) const
{
    Q_UNUSED(thisPtr);
    return ReplyType<Target>();
}
} // namespace Radapter

Q_DECLARE_METATYPE(QSharedPointer<Radapter::Reply>)
IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(Radapter::Reply, Reply::None)

#endif // RADAPTER_REPLY_H
