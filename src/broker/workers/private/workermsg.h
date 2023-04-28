#ifndef WORKERMSG_H
#define WORKERMSG_H

#include <QObject>
#include "jsondict/jsondict.h"
#include "broker/commands/basiccommands.h"
#include "broker/replies/private/reply.h"

namespace Radapter {
class Broker;
class Worker;
class WorkerProxy;
class RADAPTER_API WorkerMsg : public JsonDict {
    Q_GADGET
public:
    WorkerMsg();
    WorkerMsg(Worker *sender, const QStringList &receivers = {});
    WorkerMsg(Worker *sender, const QSet<Worker *> &receivers);
    //! Msg Flags
    enum MsgFlag : quint32 {
        MsgOk = 1 << 0, //! Valid Msg Marker
        MsgBroadcast = 1 << 1, //! Broadcast to every worker
        MsgCommand = 1 << 2,
        MsgReply = 1 << 3
    };
    Q_DECLARE_FLAGS(Flags, MsgFlag);
    Q_ENUM(MsgFlag)
    //! Used as key to serviceData() method to store extra info
    enum ServiceData : qint32 {
        ServiceUserData = -2, //! Field for user data
        ServiceUserDataDescription = -1, //! Field for user data description
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
    void ignoreReply();
    bool replyIgnored() const;
    Broker *broker();
    const QSet<Worker *> &receivers() const;
    QSet<Worker *> &receivers() noexcept;
    template<class T> inline bool isFrom() const;
    const constexpr quint64 &id() const noexcept {return m_id;}
    void setId(quint64 id);
    Worker *sender() const;
    const JsonDict &json() const;
    JsonDict &json();
    bool constexpr isBroadcast() const noexcept {return m_flags.testFlag(MsgBroadcast);}
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
    template <class User, class Slot>
    void setCallback(User *user, Slot&&slot) {
        if (!command()) {
            throw std::runtime_error("Attempt to set callback for non-command Msg!");
        }
        command()->setCallback(user, std::forward<Slot>(slot));
    }
    template <class User, class Slot>
    void setFailCallback(User *user, Slot&&slot) {
        if (!command()) {
            throw std::runtime_error("Attempt to set fail-callback for non-command Msg!");
        }
        command()->setFailCallback(user, std::forward<Slot>(slot));
    }
private:
    friend class Radapter::Worker;
    friend class Radapter::Broker;
    friend class Radapter::WorkerProxy;

    static quint64 newMsgId();
    static std::atomic<quint64> m_currentMsgId;
    void setDummy();
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

template <typename Json>
void WorkerMsg::setJson(Json &&src)
{
    static_cast<JsonDict&>(*this) = std::forward<Json>(src);
}

template<class CommandT>
void WorkerMsg::setCommand(CommandT *command)
{
    static_assert(CommandInfo<CommandT>::Defined, "Command must be registered with RADAPTER_DECLARE_COMMAND()");
    static_assert(std::is_base_of<Command, CommandT>(), "Command must inherit Radapter::Command");
    m_flags |= MsgCommand;
    m_serviceData[ServiceCommand].setValue(QSharedPointer<Command>(command));
}

template<class ReplyT>
void WorkerMsg::setReply(ReplyT *reply)
{
    static_assert(ReplyInfo<ReplyT>::Defined, "Reply must be registered with RADAPTER_DECLARE_REPLY()");
    static_assert(std::is_base_of<Reply, ReplyT>(), "Reply must inherit Radapter::Reply");
    m_flags |= MsgReply;
    setDummy();
    m_serviceData[ServiceReply].setValue(QSharedPointer<Reply>(reply));
}

}

#endif //WORKERMSG_H
