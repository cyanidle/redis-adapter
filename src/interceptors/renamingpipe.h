#ifndef RADAPTER_RENAMINGPIPE_H
#define RADAPTER_RENAMINGPIPE_H

#include "broker/interceptor/interceptor.h"
namespace Settings {
struct RenamingPipe;
}
namespace Radapter {

class RenamingPipe : public Radapter::Interceptor
{
    Q_OBJECT
    struct Private;
public:
    Interceptor *newCopy() const override;
    RenamingPipe(const Settings::RenamingPipe &settings);
    ~RenamingPipe() override;
public slots:
    void onMsgFromWorker(Radapter::WorkerMsg &msg) override;
private:
    Private *d;
};

} // namespace Radapter

#endif // RADAPTER_RENAMINGPIPE_H
