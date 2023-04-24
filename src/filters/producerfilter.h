#ifndef PRODUCERFILTER_H
#define PRODUCERFILTER_H

#include <QObject>
#include <QQueue>
#include "broker/workers/private/workermsg.h"
#include "jsondict/jsondict.h"
#include "broker/interceptor/interceptor.h"
#include "settings/settings.h"

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
