#include "rediskeyeventsconsumer.h"
#include "radapterlogging.h"

using namespace Redis;

KeyEventsConsumer::KeyEventsConsumer(const Settings::RedisKeyEventSubscriber &config, QThread *thread)
    : Connector(config, thread),
      m_keyEventNotifications(config.keyEvents)
{
    connect(this, &KeyEventsConsumer::connected, this, &KeyEventsConsumer::subscribe);
    connect(this, &KeyEventsConsumer::disconnected, this, &KeyEventsConsumer::unsubscribe);
    blockSelectDb();
}

void KeyEventsConsumer::readMessageCallback(redisReply *reply)
{
    auto parsed = parseReply(reply);
}

void KeyEventsConsumer::subscribe()
{
    if (m_keyEventNotifications.isEmpty() || m_isSubscribed) {
        return;
    }
    if (runAsyncCommand(&KeyEventsConsumer::readMessageCallback, subscribeTo(m_keyEventNotifications)) != REDIS_OK) {
        throw std::runtime_error("Incorrect Patterns for subscribtion: " + m_keyEventNotifications.join(", ").toStdString());
    }
    m_isSubscribed = true;
}

void KeyEventsConsumer::unsubscribe()
{
    m_isSubscribed = false;
}
