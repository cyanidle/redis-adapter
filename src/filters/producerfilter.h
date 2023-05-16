#ifndef PRODUCERFILTER_H
#define PRODUCERFILTER_H

#include "broker/interceptor/interceptor.h"

class JsonDict;
namespace Settings {
struct ProducerFilter;
}
namespace Radapter {
class RADAPTER_API ProducerFilter : public Radapter::Interceptor
{
    Q_OBJECT
    struct Private;
public:
    enum Strategy {
        StrategyStrict = 0,
        StrategyByWildcard = 1
    };
    explicit ProducerFilter(const Settings::ProducerFilter &settings);
    Radapter::Interceptor *newCopy() const override;
    ~ProducerFilter();
public slots:
    void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    void filterStrictByName(Radapter::WorkerMsg &msg);
    void addFiltersByWildcard(const JsonDict &cachedJson);

    Private *d;
};

}

#endif // PRODUCERFILTER_H
