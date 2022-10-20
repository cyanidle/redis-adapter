#ifndef PRODUCERFILTER_H
#define PRODUCERFILTER_H

#include <QObject>
#include <QQueue>
#include "json-formatters/formatters/dict.h"
#include "radapter-broker/interceptorbase.h"
#include "redis-adapter/settings/settings.h"
#include "redis-adapter/connectors/redisconnector.h"

class RADAPTER_SHARED_SRC DeltaFilter : public Radapter::InterceptorBase
{
    Q_OBJECT
public:
    enum Strategy {
        StrategyStrict = 0,
        StrategyByWildcard = 1
    };
    explicit DeltaFilter(const Settings::Filters::Table &filters);

signals:
    void requestCacheRead();

public slots:
    void onMsgFromWorker(const Radapter::WorkerMsg &msg) override;

private:
    void filterStrictByName(const Radapter::WorkerMsg &msg);
    void addFiltersByWildcard(const Formatters::Dict &cachedJson);

    Strategy m_strategy;
    Settings::Filters::Table m_filters;
    Settings::Filters::Table m_wildcardFilters;
    Radapter::WorkerMsg m_last;
};

#endif // PRODUCERFILTER_H
