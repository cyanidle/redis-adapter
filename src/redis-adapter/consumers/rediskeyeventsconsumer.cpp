#include "rediskeyeventsconsumer.h"
#include "redis-adapter/formatters/rediskeyeventformatter.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/include/redismessagekeys.h"
#include "redis-adapter/radapterlogging.h"

using namespace Redis;

KeyEventsConsumer::KeyEventsConsumer(const QString &host,
                                     const quint16 port,
                                     const QStringList &keyEvents,
                                     const Radapter::WorkerSettings &settings,
                                     QThread *thread)
    : Connector(host, port, 0u, settings, thread),
      m_keyEventNotifications(keyEvents),
      m_isSubscribed(false)
{
    connect(this, &KeyEventsConsumer::connected, this, &KeyEventsConsumer::subscribeToKeyEvents);
    connect(this, &KeyEventsConsumer::disconnected, this, &KeyEventsConsumer::unsubscribe);
    blockSelectDb();
    disablePingKeepalive();
}

void KeyEventsConsumer::finishMessageRead(const QVariantList &jsons)
{
    for (auto &json: jsons) {
        auto jsonDict = json.toMap();
        if (!jsonDict.isEmpty()) {
            emit sendMsg(prepareMsg(JsonDict{jsonDict}));
        }
    }
}

void KeyEventsConsumer::readMessageCallback(redisReply *reply)
{
    if (isNullReply(reply)) {
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo().c_str() << "Empty message.";
        finishMessageRead( QVariantList{});
        return;
    }

    auto message = QVariantList{};
    reDebug() << metaInfo().c_str() << "message received:" << reply->element[1]->str;
    for (quint16 i = 0; i < reply->elements; i++) {
        auto messageItem = reply->element[i];
        auto messageString = toString(messageItem);
        if (!messageString.isEmpty()) {
            message.append(messageString);
        }
    }
    finishMessageRead(message);
}

void KeyEventsConsumer::subscribeToKeyEventsImpl(const QStringList &eventTypes)
{
    if (eventTypes.isEmpty()) {
        finishMessageRead( QVariantList());
        return;
    }
    auto command = RedisQueryFormatter{}.toKeyEventsSubscribeCommand(eventTypes);
    runAsyncCommand(&KeyEventsConsumer::readMessageCallback, command);
    reDebug() << metaInfo().c_str() << "Listening to key events of types:" << eventTypes;
}

void KeyEventsConsumer::eventReceived(const QVariantList &jsonMessage)
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
