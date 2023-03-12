#ifndef WORKERMSG_H
#define WORKERMSG_H

#include <QObject>
#include "jsondict/jsondict.hpp"
#include "broker/commands/basiccommands.h"
#include "broker/replies/private/reply.h"

namespace Radapter {
class Broker;
class Worker;
class WorkerProxy;
class RADAPTER_SHARED_SRC WorkerMsg : public JsonDict {
    Q_GADGET
public:
    using Scoped = QScopedPointer<WorkerMsg>;
    WorkerMsg();
    WorkerMsg(Worker *sender, const QStringList &receivers = {});
    WorkerMsg(Worker *sender, const QSet<Worker *> &receivers);
    //! Msg Flags
    enum MsgFlag : quint32 {
        MsgBad = 0, //! Invalid Msg Marker
        MsgOk = 1 << 1, //! Valid Msg Marker
        MsgDirect = 1 << 2, //! Forward to receivers, disregarding producer/consumer status
        MsgBroadcast = 1 << 3, //! Broadcast to every worker
        MsgCommand = 1 << 4,
        MsgReply = 1 << 5
    };
    Q_DECLARE_FLAGS(Flags, MsgFlag);
    Q_ENUM(MsgFlag)
    //! Used as key to serviceData() method to store extra info
    enum ServiceData : qint32 {
        ServiceUserData = -2, //! Field for user data
        ServiceUserDataDescription = -1, //! Field for user data description
        ServiceBadReasonField, //! Contains invalidation reason
        ServiceCommand,
        ServiceReply,
        ServicePrivate,
    };
    Q_ENUM(ServiceData)
    QVariant &serviceData(ServiceData key);
    QVariant serviceData(ServiceData key) const;
    QVariant &userData();
    QVariant userData() const;
    template <typename Json>
    void setJson(Json &&src);
    bool testFlags(Flags flags) const noexcept;
    void setFlag(MsgFlag flag, bool value = true) noexcept;
    void clearFlags() noexcept;
    void clearJson();
    Broker *broker();
    const QSet<Worker *> &receivers() const;
    QSet<Worker *> &receivers() noexcept;
    template<class T> inline bool isFrom() const;
    const constexpr quint64 &id() const noexcept {return m_id;}
    Worker *sender() const;
    const JsonDict &json() const;
    JsonDict &json();
    bool constexpr isValid() const noexcept {return !m_flags.testFlag(MsgBad) && m_flags.testFlag(MsgOk);}
    bool constexpr isBroadcast() const noexcept {return m_flags.testFlag(MsgBroadcast);}
    bool constexpr isDirect() const noexcept {return m_flags.testFlag(MsgDirect);}

    bool constexpr isCommand() const noexcept {return m_flags.testFlag(MsgCommand);}
    const Command *command() const;
    Command *command();
    template <class CommandT>
    void setCommand(CommandT *command);

    bool constexpr isReply() const noexcept {return m_flags.testFlag(MsgReply);}
    const Reply *reply() const;
    Reply *reply();
    template <class ReplyT>
    void setReply(ReplyT *reply);

    QString printFullDebug() const;
    QString printFlatData() const;
    QString printFlags() const;
    JsonDict printServiceData() const;
    QStringList printReceivers() const;
    QVariant &privateData();
    QVariant privateData() const;
    template <class User, class...Args>
    void setCallback(User *user, void (User::*cb)(Args...)) {
        if (!command()) {
            throw std::runtime_error("Attempt to set callback for non-command Msg!");
        }
        command()->setCallback(user, cb);
    }
private:
    friend class Radapter::Worker;
    friend class Radapter::Broker;
    friend class Radapter::WorkerProxy;

    static quint64 newMsgId();
    static std::atomic<quint64> m_currentMsgId;

    void updateId();

    Flags m_flags;
    Worker *m_sender;
    quint64 m_id;
    QSet<Worker*> m_receivers;
    QHash<ServiceData, QVariant> m_serviceData;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(Radapter::WorkerMsg::Flags)
Q_DECLARE_METATYPE(Radapter::WorkerMsg);
Q_DECLARE_TYPEINFO(Radapter::WorkerMsg::ServiceData, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Radapter::WorkerMsg::MsgFlag, Q_PRIMITIVE_TYPE);
Q_DECLARE_TYPEINFO(Radapter::WorkerMsg, Q_MOVABLE_TYPE);

namespace Radapter {
template<class T>
inline bool Radapter::WorkerMsg::isFrom() const {
    static_assert(std::is_base_of<Worker, T>(),
                  "Attempt to check sender for non WorkerBase subclass!");
    return qobject_cast<const T*>(m_sender) != nullptr;
}

inline const QSet<Radapter::Worker*> &Radapter::WorkerMsg::receivers() const
{
    return m_receivers;
}

inline QVariant &WorkerMsg::serviceData(ServiceData key)
{
    return m_serviceData[key];
}

inline QVariant WorkerMsg::serviceData(ServiceData key) const
{
    return m_serviceData.value(key);
}

inline QVariant &WorkerMsg::userData()
{
    return serviceData(ServiceUserData);
}

inline QVariant WorkerMsg::userData() const
{
    return serviceData(ServiceUserData);
}

inline bool WorkerMsg::testFlags(Flags flags) const noexcept
{
    return m_flags & flags;
}

template <typename Json>
void WorkerMsg::setJson(Json &&src)
{
    static_cast<JsonDict&>(*this) = std::forward<Json>(src);
}

inline void WorkerMsg::setFlag(MsgFlag flag, bool value) noexcept
{
    m_flags.setFlag(flag, value);
}

inline void WorkerMsg::clearFlags() noexcept
{
    m_flags = Flags(MsgOk);
}

inline void WorkerMsg::clearJson()
{
    m_dict.clear();
}

inline QSet<Worker *> &WorkerMsg::receivers() noexcept
{
    return m_receivers;
}

inline Worker *WorkerMsg::sender() const
{
    return m_sender;
}

inline const JsonDict &WorkerMsg::json() const
{
    return *this;
}

inline JsonDict &WorkerMsg::json() {
    return *this;
}

inline const Command *WorkerMsg::command() const
{
    return m_serviceData[ServiceCommand].value<QSharedPointer<Command>>().data();
}

inline Command *WorkerMsg::command()
{
    return m_serviceData[ServiceCommand].value<QSharedPointer<Command>>().data();
}

inline const Reply *WorkerMsg::reply() const
{
    return m_serviceData[ServiceReply].value<QSharedPointer<Reply>>().data();
}

inline Reply *WorkerMsg::reply()
{
    return m_serviceData[ServiceReply].value<QSharedPointer<Reply>>().data();
}

inline QVariant &WorkerMsg::privateData()
{
    return m_serviceData[ServicePrivate];
}

inline QVariant WorkerMsg::privateData() const
{
    return m_serviceData[ServicePrivate];
}

inline void WorkerMsg::updateId()
{
    m_id = newMsgId();
}

inline quint64 WorkerMsg::newMsgId()
{
    return m_currentMsgId.fetch_add(1, std::memory_order_relaxed);
}

inline QDebug operator<<(QDebug dbg, const Radapter::WorkerMsg &msg){
    dbg.nospace().noquote() << msg.printFullDebug();
    return dbg.maybeQuote().maybeSpace();
}

template<class CommandT>
void WorkerMsg::setCommand(CommandT *command)
{
    static_assert(CommandInfo<CommandT>::Defined, "Command must be registered with RADAPTER_DECLARE_COMMAND()");
    static_assert(std::is_base_of<Command, CommandT>(), "Command must inherit Radapter::Command");
    m_flags |= MsgDirect;
    m_flags |= MsgCommand;
    m_serviceData[ServiceCommand].setValue(QSharedPointer<Command>(command));
}

template<class ReplyT>
void WorkerMsg::setReply(ReplyT *reply)
{
    static_assert(ReplyInfo<ReplyT>::Defined, "Reply must be registered with RADAPTER_DECLARE_REPLY()");
    static_assert(std::is_base_of<Reply, ReplyT>(), "Reply must inherit Radapter::Reply");
    m_flags |= MsgDirect;
    m_flags |= MsgReply;
    if (!command()) {
        setCommand(new CommandDummy);
    }
    m_serviceData[ServiceReply].setValue(QSharedPointer<Reply>(reply));
}

}

#endif //WORKERMSG_H
