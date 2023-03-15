#ifndef COMMANDREPLYMACROS_H
#define COMMANDREPLYMACROS_H

#include "global.h"

#define GADGET_BODY \
    virtual const QMetaObject *metaObject() const override {return &this->staticMetaObject;};

#define RADAPTER_DECLARE_COMMAND(command) \
    namespace Radapter{ \
    template<> \
    struct CommandInfo<command> : public CommandInfoSpecialized<command> {}; \
}
#define IMPL_RADAPTER_DECLARE_COMMAND_BUILTIN(command, enumName) \
    namespace Radapter{ \
    template<> \
    struct CommandInfo<command> : public CommandInfoSpecialized<command, enumName> {}; \
}

#define RADAPTER_DECLARE_REPLY(reply) \
    namespace Radapter{ \
    template<> \
    struct ReplyInfo<reply> : public ReplyInfoSpecialized<reply>{}; \
}
#define IMPL_RADAPTER_DECLARE_REPLY_BUILTIN(reply, enumName) \
    namespace Radapter{ \
    template<> \
    struct ReplyInfo<reply> : public ReplyInfoSpecialized<reply, enumName>{}; \
}

#define RADAPTER_COMMAND_WANTS(wanted_reply) \
    GADGET_BODY \
    using WantedReply = wanted_reply; \
    static const wanted_reply *replyCast(const Radapter::Reply* reply) { \
        return Radapter::reply_cast<const wanted_reply *>(reply); \
    } \
    template <typename... Args> \
    static wanted_reply *makeReply(Args&&...args) { \
        return new wanted_reply(std::forward<Args>(args)...); \
    } \
    virtual Radapter::Reply::Type wantedReplyType() const override { \
        return Radapter::ReplyType<wanted_reply>(); \
    } \
    virtual const QMetaObject *wantedReplyMetaObject() const override { \
        return &wanted_reply::staticMetaObject; \
    } \
    virtual Radapter::Command* newCopy() const override { \
        auto copy = new Radapter::stripped_this<decltype(this)>(*this); \
        return copy;\
    }

#define RADAPTER_REPLY \
    GADGET_BODY \
    virtual Radapter::Reply* newCopy() const override { \
        auto copy = new Radapter::stripped_this<decltype(this)>(*this); \
        return copy;\
    }

#endif // COMMANDREPLYMACROS_H
