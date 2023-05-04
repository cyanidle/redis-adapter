#ifndef RADAPTER_REMAPPINGPIPE_H
#define RADAPTER_REMAPPINGPIPE_H

#include "broker/interceptor/interceptor.h"

namespace Settings {struct RemappingPipe;}
namespace Radapter {

class RemappingPipe : public Radapter::Interceptor
{
    Q_OBJECT
    struct Private;
public:
    Interceptor *newCopy() const override;
    RemappingPipe(const Settings::RemappingPipe &settings);
    ~RemappingPipe();
public slots:
    void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    Private *d;
};

} // namespace Radapter

#endif // RADAPTER_REMAPPINGPIPE_H
