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
    const QString &baseFilepath() const {return m_settings.filepath;}
    QString currentFilepath() const {return m_file->fileName();}
    bool isFull() const;
public slots:
    virtual void onMsg(const Radapter::WorkerMsg &msg) override;
    virtual void onCommand(const Radapter::WorkerMsg &msg) override;
    virtual void onReply(const Radapter::WorkerMsg &msg) override;
private slots:
    void onFlush();
private:
    bool testMsgForLog(const Radapter::WorkerMsg &msg);
    void enqueueMsg(const WorkerMsg &msg);

    QFile *m_file;
    QTimer *m_flushTimer;
    LoggingWorkerSettings m_settings;
    QJsonArray m_array;
    bool m_shouldUpdate{false};
};
#endif
