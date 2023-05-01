#ifndef RADAPTER_METAINFOPIPE_H
#define RADAPTER_METAINFOPIPE_H

#include "broker/interceptor/interceptor.h"

namespace Radapter {

class MetaInfoPipe : public Radapter::Interceptor
{
    Q_OBJECT
public:
    MetaInfoPipe();
    Interceptor *newCopy() const override;
public slots:
    void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
};

} // namespace Radapter

#endif // RADAPTER_METAINFOPIPE_H
