#ifndef RADAPTER_COMMAND_H
#define RADAPTER_COMMAND_H

#include <QString>
#include <QSharedPointer>
#include <limits>
#include "private/global.h"
#include "private/commandreplymacros.h"
#include "broker/replies/private/reply.h"
#include "commandcallback.h"

namespace Radapter {

class RADAPTER_API Command
{
    Q_GADGET
public:
    enum Type : quint32 {
        None = 0,
        Acknowledge,
        RequestJson,
        Dummy,
        Trigger,

        Pack = 100,

        User = 500,
        MaxUser = std::numeric_limits<std::underlying_type<Type>::type>::max(),
    };
    bool isUser() const;
    Type type() const;
    virtual const QMetaObject *metaObject() const;
    using WantedReply = Reply;
    template <typename Target> bool is() const;
    template <typename Target> Target *as();
    template <typename Target> const Target *as() const;
    template <typename Target> bool inherits() const;
    bool isWantedReply(const Reply *reply) const;
    virtual bool replyOk(const Radapter::Reply* reply) const;
    static quint32 generateUserType();
    virtual Command* newCopy() const = 0;
    virtual Reply::Type wantedReplyType() const = 0;
    virtual const QMetaObject *wantedReplyMetaObject() const = 0;
    virtual ~Command() = default;
    void *voidCast(const QMetaObject* meta);
    const void *voidCast(const QMetaObject* meta) const;
    void setCallback(const CommandCallback &cb);
    void setFailCallback(const CommandCallback &cb);
    template <class User, class Slot>
    void setCallback(User *user, Slot slot) {
        setCallback(CommandCallback::fromAny(user, slot));
    }
    template <class User, class Slot>
    void setFailCallback(User *user, Slot slot) {
        setFailCallback(CommandCallback::fromAny(user, slot));
    }
    const CommandCallback &callback() const;
    CommandCallback &callback();
    const CommandCallback &failCallback() const;
    CommandCallback &failCallback();
protected:
    Command(quint32 type);
    template <typename Target> static Type typeInConstructor(const Target* thisPtr);
private:
    CommandCallback m_cb;
    CommandCallback m_failCb;
    quint32 m_type;
};

template <typename Target>
struct CommandInfo {
    using WantedReply = Reply;
    static Command::Type type() {
        return Command::None;
    }
    static const QMetaObject *metaObject() {
        return nullptr;
    }
    static bool isWantedReply(const Reply* reply) {
        Q_UNUSED(reply)
        return false;
    }
    static Reply *toWantedReply(const Reply* reply) {
        Q_UNUSED(reply)
        return nullptr;
    }
    enum {
        Defined = false,
        IsBuiltIn = false
    };
};

template <typename Target>
Command::Type CommandType() {
    static_assert(CommandInfo<Target>::Defined, "Use RADAPTER_DECLARE_COMMAND(<YourCommand>)");
    return CommandInfo<Target>::type();
}

template <typename WantedCommand>
WantedCommand command_cast(const Command *command) {
    using decayed = typename std::remove_cv<typename std::remove_pointer<WantedCommand>::type>::type;
    static_assert(CommandInfo<decayed>::Defined, "Use RADAPTER_DECLARE_COMMAND(<YourCommand>)");
    if (command && CommandType<decayed>() == command->type()) {
        return static_cast<WantedCommand>(command);
    }
    return static_cast<WantedCommand>(command->voidCast(&decayed::staticMetaObject));
}
template <typename WantedCommand>
WantedCommand command_cast(Command *command) {
    using decayed = typename std::remove_cv<typename std::remove_pointer<WantedCommand>::type>::type;
    static_assert(CommandInfo<decayed>::Defined, "Use RADAPTER_DECLARE_COMMAND(<YourCommand>)");
    if (command && CommandType<decayed>() == command->type()) {
        return static_cast<WantedCommand>(command);
    }
    return static_cast<WantedCommand>(command->voidCast(&decayed::staticMetaObject));
}

template <typename T, class = void>
struct has_wanted_reply : std::false_type{};
template <typename T>
struct has_wanted_reply<T, std::void_t<
        typename T::WantedReply
        >> : std::true_type{};

template <typename Target, Command::Type builtin = Command::Type::None>
struct CommandInfoSpecialized {
    static_assert(has_wanted_reply<Target>::value && Radapter::Gadget_With_MetaObj<Target>::Value,
        "Place in class body RADAPTER_COMMAND_WANTS(<wanted reply>) in public: section");
    using WantedReply = typename Target::WantedReply;
    static Command::Type type() {
        static quint32 type{IsBuiltIn ? builtin : Command::generateUserType()};;
        return static_cast<Command::Type>(type);
    }
    static const QMetaObject *metaObject() {
        return &Target::staticMetaObject;
    }
    static bool isWantedReply(const Reply* reply) {
        return ReplyType<WantedReply>() == reply->type();
    }
    static WantedReply *toWantedReply(const Reply* reply) {
        return isWantedReply(reply) ? static_cast<const WantedReply *>(reply) : nullptr;
    }
    enum {
        Defined = true,
        IsBuiltIn = builtin != Command::None
    };
};

template <typename Target> Target *Command::as()
{
    return command_cast<Target *>(this);
}
template <typename Target> const Target *Command::as() const
{
    return command_cast<const Target *>(this);
}
template<typename Target>
bool Command::is() const
{
    return type() == CommandType<Target>() || inherits<Target>();
}

template<typename Target>
bool Command::inherits() const
{
    return metaObject()->inherits(CommandInfo<Target>::metaObject());
}

template<typename Target>
Command::Type Command::typeInConstructor(const Target* thisPtr)
{
    Q_UNUSED(thisPtr);
    return CommandType<Target>();
}

} // namespace Radapter

Q_DECLARE_METATYPE(QSharedPointer<Radapter::Command>)
IMPL_RADAPTER_DECLARE_COMMAND_BUILTIN(Radapter::Command, Command::None)

#endif // RADAPTER_COMMAND_H
