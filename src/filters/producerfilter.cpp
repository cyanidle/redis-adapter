#include "producerfilter.h"

ProducerFilter::ProducerFilter(const Settings::Filters::Table &filters)
    : Interceptor(),
      m_strategy(StrategyStrict),
      m_filters(filters),
      m_last()
{
    for (auto filter = m_filters.begin();
         filter != m_filters.end();
         filter++)
    {
        if (filter.key().contains("*")) {
            m_wildcardFilters.insert(filter.key(), filter.value());
            m_strategy = StrategyByWildcard;
            break;
        }
    }
}

void ProducerFilter::onMsgFromWorker(const Radapter::WorkerMsg &msg)
{
    if (msg.testFlags(Radapter::WorkerMsg::MsgBad |
                      Radapter::WorkerMsg::MsgReply |
                      Radapter::WorkerMsg::MsgCommand)) {
        emit msgToBroker(msg);
        return;
    }
    if (m_strategy == StrategyByWildcard) {
        addFiltersByWildcard(msg);
    }
    filterStrictByName(msg);
    m_last.merge(msg);
}

void ProducerFilter::filterStrictByName(const Radapter::WorkerMsg &srcMsg)
{
    if (m_last.isEmpty()) {
        emit msgToBroker(srcMsg);
    }
    bool shouldAdd = false;
    for (auto &item : srcMsg) {
        auto currentValue = item.value();
        auto lastValue = m_last[item.key()];
        auto currentKey = item.key().join(":");
        if (!m_filters.contains(currentKey)) {
            shouldAdd = true;
            break;
        }
        if (!currentValue.canConvert<double>() || !lastValue.canConvert<double>()) {
            shouldAdd = true;
            break;
        }
        if (qAbs(currentValue.toDouble() - lastValue.toDouble()) > m_filters[currentKey]) {
            shouldAdd = true;
            break;
        }
    }
    if (shouldAdd) {
        emit msgToBroker(srcMsg);
    }
}

void ProducerFilter::addFiltersByWildcard(const JsonDict &cachedJson)
{
    auto cachedKeys = cachedJson.flatten(":").keys();
    for (auto &key : cachedKeys) {
        if (m_filters.contains(key)) {
            continue;
        }
        for (auto filter = m_wildcardFilters.begin();
             filter != m_wildcardFilters.end();
             filter++)
        {
            auto wildcardKey = filter.key().split("*").first();
            if (key.startsWith(wildcardKey)) {
                m_filters.insert(key, filter.value());
            }
        }
    }
}
