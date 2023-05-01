#ifndef LOGGING_INTERCEPTOR_H
#define LOGGING_INTERCEPTOR_H

#include "private/global.h"
#include "worker.h"
class QFile;
class QThread;
namespace Settings {
struct FileWorker;
}
namespace Radapter {
class RADAPTER_API FileWorker : public Radapter::Worker
{
    Q_OBJECT
    struct Private;
public:
    explicit FileWorker(const Settings::FileWorker &settings, QThread *thread);
    const QString &filepath() const;
    ~FileWorker();
public slots:
    virtual void onMsg(const Radapter::WorkerMsg &msg) override;
private:
    void startReading();
    void appendToFile(const JsonDict& info);
    bool checkOpened();

    Private *d;
};
}

#endif
