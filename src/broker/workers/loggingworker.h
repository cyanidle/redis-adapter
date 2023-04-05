#ifndef LOGGING_INTERCEPTOR_H
#define LOGGING_INTERCEPTOR_H

#include <QTimer>
#include <QFile>
#include "private/global.h"
#include "loggingworkersettings.h"
#include "worker.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QMutex>

namespace Radapter {
class RADAPTER_API LoggingWorker;
}

class Radapter::LoggingWorker : public Radapter::Worker
{
    Q_OBJECT
public:
    explicit LoggingWorker(const LoggingWorkerSettings &settings, QThread *thread);
    const QString &baseFilepath() const;
    QString currentFilepath() const;
public slots:
    virtual void onMsg(const Radapter::WorkerMsg &msg) override;
    virtual void onCommand(const Radapter::WorkerMsg &msg) override;
    virtual void onReply(const Radapter::WorkerMsg &msg) override;
private:
    void appendToFile(const JsonDict& info);
    bool testMsgForLog(const Radapter::WorkerMsg &msg);

    QFile *m_file;
    LoggingWorkerSettings m_settings;
};
#endif
