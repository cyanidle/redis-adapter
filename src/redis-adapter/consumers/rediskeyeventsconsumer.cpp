#include "rediskeyeventsconsumer.h"
#include "redis-adapter/formatters/rediskeyeventformatter.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/include/redismessagekeys.h"
#include "redis-adapter/radapterlogging.h"

using namespace Redis;

KeyEventsConsumer::KeyEventsConsumer(const QString &host,
                                     const quint16 port,
                                     const QStringList &keyEvents,
                                     const Radapter::WorkerSettings &settings)
    : RedisConnector(host, port, 0u, settings),
      m_keyEventNotifications(keyEvents),
      m_isSubscribed(false)
{
}

void KeyEventsConsumer::finishMessageRead(const Formatters::List &jsons)
{
    for (auto &json: jsons) {
        auto jsonDict = json.toMap();
        if (!jsonDict.isEmpty()) {
            emit sendMsg(prepareMsg(jsonDict));
        }
    }
}

void KeyEventsConsumer::readMessageCallback(redisAsyncContext *context, void *replyPtr, void *sender)
{
    if (isNullReply(context, replyPtr, sender)) {
        return;
    }
    auto reply = static_cast<redisReply *>(replyPtr);
    auto adapter = static_cast<KeyEventsConsumer *>(sender);
    if (reply->elements == 0) {
        reDebug() << metaInfo(context).c_str() << "Empty message.";
        adapter->finishMessageRead(Formatters::List{});
        return;
    }

    auto message = Formatters::List{};
    reDebug() << metaInfo(context).c_str() << "message received:" << reply->element[1]->str;
    for (quint16 i = 0; i < reply->elements; i++) {
        auto messageItem = reply->element[i];
        auto messageString = toString(messageItem);
        if (!messageString.isEmpty()) {
            message.append(messageString);
        }
    }
    adapter->finishMessageRead(message);
}

void KeyEventsConsumer::subscribeToKeyEventsImpl(const QStringList &eventTypes)
{
    if (eventTypes.isEmpty()) {
        finishMessageRead(Formatters::List());
        return;
    }
    auto command = RedisQueryFormatter{}.toKeyEventsSubscribeCommand(eventTypes);
    runAsyncCommand(readMessageCallback, command);
    reDebug() << metaInfo().c_str() << "Listening to key events of types:" << eventTypes;
}

void KeyEventsConsumer::run()
{
    connect(this, &KeyEventsConsumer::connected, this, &KeyEventsConsumer::subscribeToKeyEvents);
    connect(this, &KeyEventsConsumer::disconnected, this, &KeyEventsConsumer::unsubscribe);
    blockSelectDb();
    disablePingKeepalive();
    RedisConnector::run();
}

void KeyEventsConsumer::eventReceived(const Formatters::List &jsonMessage)
{
    auto jsonEvent = RedisKeyEventFormatter(jsonMessage).toEventMessage();
    if (!jsonEvent.isEmpty()) {
        emit sendMsg(prepareMsg(jsonEvent));
    }
}

void KeyEventsConsumer::subscribeToKeyEvents()
{
    if (m_keyEventNotifications.isEmpty() || m_isSubscribed) {
        return;
    }
    subscribeToKeyEventsImpl(m_keyEventNotifications);
    m_isSubscribed = true;
}

void KeyEventsConsumer::unsubscribe()
{
    m_isSubscribed = false;
}
