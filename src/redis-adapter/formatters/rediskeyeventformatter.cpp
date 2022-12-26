#include "rediskeyeventformatter.h"
#include "redis-adapter/include/redismessagekeys.h"

RedisKeyEventFormatter::RedisKeyEventFormatter(const QVariantList &message, QObject *parent)
    : QObject(parent),
      m_message{}
{
    if (message.isEmpty()
            || (message.count() != REDIS_PMESSAGE_SIZE))
    {
        return;
    }
    auto messageType = message.at(REDIS_MESSAGE_INDEX_TYPE).toString();
    auto messagePattern = message.at(REDIS_PMESSAGE_INDEX_PATTERN).toString();
    if ((messageType == REDIS_MESSAGE_TYPE_PATTERN)
            && (messagePattern.startsWith(REDIS_PATTERN_KEY_EVENT)))
    {
        m_message = message;
    }
}

QString RedisKeyEventFormatter::eventType() const
{
    if (m_message.isEmpty()) {
        return QString{};
    }
    auto pattern = m_message.at(REDIS_PMESSAGE_INDEX_PATTERN).toString();
    auto eventType = pattern.split(REDIS_SEP).last();
    return eventType;
}

QString RedisKeyEventFormatter::eventKey() const
{
    if (m_message.isEmpty()) {
        return QString{};
    }
    auto payload = m_message.at(REDIS_PMESSAGE_INDEX_PAYLOAD).toString();
    return payload;
}

 JsonDict RedisKeyEventFormatter::toEventMessage() const
{
    if (m_message.isEmpty()) {
        return JsonDict{};
    }
    auto jsonEvent = JsonDict{ QVariantMap{ { eventType(), eventKey() } } };
    return jsonEvent;
}
