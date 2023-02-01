#include "rediskeyeventsconsumer.h"
#include "redis-adapter/formatters/rediskeyeventformatter.h"
#include "redis-adapter/formatters/redisqueryformatter.h"
#include "redis-adapter/include/redismessagekeys.h"
#include "redis-adapter/radapterlogging.h"

using namespace Redis;

KeyEventsConsumer::KeyEventsConsumer(const Settings::RedisKeyEventSubscriber &config, QThread *thread)
    : Connector(config, thread),
      m_keyEventNotifications(config.keyEvents)
{
    connect(this, &KeyEventsConsumer::connected, this, &KeyEventsConsumer::subscribeToKeyEvents);
    connect(this, &KeyEventsConsumer::disconnected, this, &KeyEventsConsumer::unsubscribe);
    blockSelectDb();
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
    if (!isValidReply(reply)) {
        return;
    }
    if (reply->elements == 0) {
        reDebug() << metaInfo() << "Empty message.";
        finishMessageRead( QVariantList{});
        return;
    }

    auto message = QVariantList{};
    reDebug() << metaInfo() << "message received:" << reply->element[1]->str;
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
    bool bypassTracking = true;
    runAsyncCommand(&KeyEventsConsumer::readMessageCallback, command, bypassTracking);
    reDebug() << metaInfo() << "Listening to key events of types:" << eventTypes;
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
