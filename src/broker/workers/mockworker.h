#ifndef MOCK_WORKER_H
#define MOCK_WORKER_H

#include <QTimer>
#include <QFile>
#include "worker.h"
#include "private/global.h"
#include "jsondict/jsondict.h"

namespace Settings {
struct MockWorker;
}
namespace Radapter {

class RADAPTER_API MockWorker : public Radapter::Worker
{
    Q_OBJECT
public:
    explicit MockWorker(const Settings::MockWorker &settings, QThread *thread);
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
    QList<JsonDict> m_jsons;
    int m_currentIndex;
};
}

#endif
