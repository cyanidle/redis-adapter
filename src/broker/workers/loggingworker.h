#ifndef LOGGING_INTERCEPTOR_H
#define LOGGING_INTERCEPTOR_H

#include "private/global.h"
#include "worker.h"

namespace Settings {
struct LoggingWorker;
}
namespace Radapter {
struct LoggingWorkerPrivate;
class RADAPTER_API LoggingWorker : public Radapter::Worker
{
    Q_OBJECT
public:
    explicit LoggingWorker(const Settings::LoggingWorker &settings, QThread *thread);
    const QString &baseFilepath() const;
    QString currentFilepath() const;
    ~LoggingWorker();
public slots:
    virtual void onMsg(const Radapter::WorkerMsg &msg) override;
    virtual void onCommand(const Radapter::WorkerMsg &msg) override;
    virtual void onReply(const Radapter::WorkerMsg &msg) override;
private:
    void appendToFile(const JsonDict& info);
    bool testMsgForLog(const Radapter::WorkerMsg &msg);

    LoggingWorkerPrivate *d;
};
}

#endif
