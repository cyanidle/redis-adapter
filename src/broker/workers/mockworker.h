#ifndef MOCK_WORKER_H
#define MOCK_WORKER_H

#include "private/global.h"
#include "worker.h"

namespace Settings {
struct MockWorker;
}
namespace Radapter {

class RADAPTER_API MockWorker : public Radapter::Worker
{
    Q_OBJECT
    struct Private;
public:
    explicit MockWorker(const Settings::MockWorker &settings, QThread *thread);
    ~MockWorker();
    void onRun() override;
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void onReply(const Radapter::WorkerMsg &msg) override;
    void onCommand(const Radapter::WorkerMsg &msg) override;
private slots:
    void onMock();
private:
    const JsonDict &getNext();
    Private *d;
};
}

#endif
