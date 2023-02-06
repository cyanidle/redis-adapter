#ifndef PRODUCERFILTER_H
#define PRODUCERFILTER_H

#include <QObject>
#include <QQueue>
#include "jsondict/jsondict.hpp"
#include "broker/interceptors/interceptor.h"
#include "settings/settings.h"

class RADAPTER_SHARED_SRC ProducerFilter : public Radapter::InterceptorBase
{
    Q_OBJECT
public:
    enum Strategy {
        StrategyStrict = 0,
        StrategyByWildcard = 1
    };
    explicit ProducerFilter(const Settings::Filters::Table &filters);

signals:
    void requestCacheRead();

public slots:
    void onMsgFromWorker(const Radapter::WorkerMsg &msg) override;

private:
    void filterStrictByName(const Radapter::WorkerMsg &msg);
    void addFiltersByWildcard(const JsonDict &cachedJson);

    Strategy m_strategy;
    Settings::Filters::Table m_filters;
    Settings::Filters::Table m_wildcardFilters;
    Radapter::WorkerMsg m_last;
};

#endif // PRODUCERFILTER_H
