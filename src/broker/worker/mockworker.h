#ifndef MOCK_WORKER_H
#define MOCK_WORKER_H

#include <QTimer>
#include <QFile>
#include "mockworkersettings.h"
#include "worker.h"
#include "private/global.h"


namespace Radapter {
class RADAPTER_API MockWorker;
}

class Radapter::MockWorker : public Radapter::Worker
{
    Q_OBJECT
public:
    explicit MockWorker(const MockWorkerSettings &settings, QThread *thread);
    void onRun() override;
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
    void onReply(const Radapter::WorkerMsg &msg) override;
    void onCommand(const Radapter::WorkerMsg &msg) override;
private slots:
    void onMock();
private:
    const JsonDict &getNext();

    QFile* m_file;
    QTimer* m_mockTimer;
    QList< JsonDict> m_jsons;
    int m_currentIndex;
};

#endif
