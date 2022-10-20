#include "producerfilter.h"

DeltaFilter::DeltaFilter(const Settings::Filters::Table &filters)
    : InterceptorBase(),
      m_strategy(StrategyStrict),
      m_filters(filters),
      m_last()
{
    bool hasWildcard = false;
    for (auto filter = m_filters.begin();
         filter != m_filters.end();
         filter++)
    {
        if (filter.key().contains("*")) {
            m_wildcardFilters.insert(filter.key(), filter.value());
            hasWildcard = true;
        }
    }
    if (hasWildcard) {
        m_strategy = StrategyByWildcard;
    }
}

void DeltaFilter::onMsgFromWorker(const Radapter::WorkerMsg &msg)
{
    if (msg.brokerFlags == Radapter::WorkerMsg::BrokerBadMsg ||
        msg.workerFlags == Radapter::WorkerMsg::WorkerInternalCommand ||
        msg.workerFlags == Radapter::WorkerMsg::WorkerReply) {
        emit msgToBroker(msg);
        return;
    }
    if (m_strategy == StrategyByWildcard) {
        addFiltersByWildcard(msg.data());
    }
    filterStrictByName(msg);
    m_last.merge(msg);
}

void DeltaFilter::filterStrictByName(const Radapter::WorkerMsg &srcMsg)
{
    if (m_last.isEmpty()) {
        emit msgToBroker(srcMsg);
    }
    bool shouldAdd = false;
    for (auto &item : srcMsg) {
        auto currentValue = item.value();
        auto lastValue = m_last[item.getFullKey()];
        auto currentKey = item.getFullKey().join(":");
        if (!(currentValue.canConvert<double>() &&
              lastValue.canConvert<double>() &&
              m_filters.contains(currentKey))) {
            reDebug() << "Cannot filter msg from: " << srcMsg.sender;
            reDebug() << "currentValue.canConvert<double>() = " << currentValue.canConvert<double>();\
            reDebug() << "lastValue.canConvert<double>() = " << lastValue.canConvert<double>();
            reDebug() << "m_filters.contains(currentKey)) = " << m_filters.contains(currentKey);
            reDebug() << "currentKey = " << currentKey;
            emit msgToBroker(srcMsg);
            return;
        }
        if (qAbs(currentValue.toDouble() - lastValue.toDouble()) > m_filters[currentKey]) {
            shouldAdd = true;
        }
    }
    if (shouldAdd) {
        emit msgToBroker(srcMsg);
    }
}

void DeltaFilter::addFiltersByWildcard(const Formatters::Dict &cachedJson)
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
