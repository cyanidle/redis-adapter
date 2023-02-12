#ifndef REDISCACHEPRODUCER_H
#define REDISCACHEPRODUCER_H

#include <QObject>
#include "connectors/redisconnector.h"
#include "jsondict/jsondict.hpp"
#include "async_context/rediscachecontext.h"

namespace Redis {
class RADAPTER_SHARED_SRC CacheProducer;
}

class Redis::CacheProducer : public Connector
{
    Q_OBJECT
public:
    using Handle = Radapter::ContextBase::Handle;
    explicit CacheProducer(const Settings::RedisCacheProducer &config, QThread *thread);
signals:

public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void onCommand(const Radapter::WorkerMsg &msg) override;
protected:
    CacheContext &getCtx(Handle handle);
private:
    void handleCommand(Radapter::Command *command, Handle handle);
    void requestMultiple(const Radapter::CommandPack *pack, Handle handle);

    void writeKeys(const QVariantMap &keys, Handle handle);
    void msetCallback(redisReply *replyPtr, Handle handle);

    void writeObject(const QString &indexKey, const JsonDict &json, Handle handle);
    void objectWriteCallback(redisReply *reply, Handle handle);

    void writeSet(const QString& set, const QStringList &keys, Handle handle);
    void writeSetCallback(redisReply *reply, Handle handle);

    void del(const QString& target, Handle handle);
    void delCallback(redisReply *reply, Handle handle);


    QString m_objectKey;
    friend CacheContext;
    Radapter::ContextManager<CacheContext> m_manager;
};

#endif // REDISCACHEPRODUCER_H
