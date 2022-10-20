#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QObject>
#include <QMutex>
#include "radapter-broker/workermsg.h"
#include "radapter-broker/workerbase.h"


namespace Radapter {
class RADAPTER_SHARED_SRC Protocol;
class RADAPTER_SHARED_SRC CommandBase;
class RADAPTER_SHARED_SRC CommandDummy;
class RADAPTER_SHARED_SRC CommandRequestKeys;
class RADAPTER_SHARED_SRC CommandRequestJson;
class RADAPTER_SHARED_SRC CommandAcknowledgeJson;
}

class Radapter::CommandBase {
public:
    friend class Protocol;
    bool supports(const WorkerMsg &msg) const {return supports(msg.senderType);}
    bool supports(WorkerMsg::SenderType senderType) const {return supports().contains(senderType);}
    virtual QList<WorkerMsg::SenderType> supports() const = 0;
    virtual QVariant receive(const WorkerMsg &msg) const = 0;
    virtual WorkerMsg send(WorkerBase* sender, const QVariant &src) const = 0;
protected:
    WorkerMsg prepareMsg(WorkerBase* sender) const {return sender->prepareMsg();}
    WorkerMsg prepareCommand(WorkerBase* sender) const {return sender->prepareCommand();}
    WorkerMsg prepareReply(WorkerBase* sender) const {return sender->prepareReply();}
    WorkerMsg prepareMsgBroken(WorkerBase* sender, const QString &reason) const {return sender->prepareMsgBroken(reason);}
    CommandBase() {};
};

class Radapter::CommandDummy : public CommandBase {
    QList<WorkerMsg::SenderType> supports() const override {return {};}
    WorkerMsg send(WorkerBase* sender, const QVariant &src) const override;
    QVariant receive(const WorkerMsg &msg) const override;
};

class Radapter::CommandAcknowledgeJson : public CommandBase {
public:
    QList<WorkerMsg::SenderType> supports() const override;
    WorkerMsg send(WorkerBase* sender, const QVariant &src) const override;
    QVariant receive(const WorkerMsg &msg) const override;
};

class Radapter::CommandRequestJson : public CommandBase {
public:
    QList<WorkerMsg::SenderType> supports() const override;
    WorkerMsg send(WorkerBase* sender, const QVariant &src = {}) const override;
    QVariant receive(const WorkerMsg &msg) const override;
};

class Radapter::CommandRequestKeys : public CommandBase {
public:
    QList<WorkerMsg::SenderType> supports() const override;
    WorkerMsg send(WorkerBase* sender, const QVariant &src) const override;
    QVariant receive(const WorkerMsg &msg) const override;
};

class Radapter::Protocol : private QObject
{
    Q_OBJECT
public:
    static Protocol* instance() {
        QMutexLocker locker(&m_mutex);
        if (m_instance == nullptr) {
            m_instance = new Protocol();
        }
        return m_instance;
    }

    const CommandAcknowledgeJson* acknowledge() {QMutexLocker locker(&m_mutex); return m_ack;}
    const CommandRequestKeys* requestKeys() {QMutexLocker locker(&m_mutex); return m_keys;}
    const CommandRequestJson* requestNewJson() {QMutexLocker locker(&m_mutex); return m_json;}
    QList<const CommandBase*> allCustoms() {QMutexLocker locker(&m_mutex); return m_customCommands.values();}
    const CommandBase* custom(const QString &name);
    void addCustomHandler(const CommandBase* handler, const QString &name) {QMutexLocker locker(&m_mutex); m_customCommands.insert(name, handler);}

signals:

private:
    explicit Protocol();
    static QMutex m_mutex;
    static Protocol* m_instance;
    static QMap<QString, const CommandBase*> m_customCommands;
    static CommandAcknowledgeJson* m_ack;
    static CommandRequestJson* m_json;
    static CommandRequestKeys* m_keys;
    static CommandDummy* m_dummy;
};

#endif // PROTOCOL_H
