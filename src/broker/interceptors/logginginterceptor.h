#ifndef LOGGING_INTERCEPTOR_H
#define LOGGING_INTERCEPTOR_H

#include <QTimer>
#include <QFile>
#include "private/global.h"
#include "logginginterceptorsettings.h"
#include "broker/interceptors/interceptor.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QMutex>

namespace Radapter {
class RADAPTER_API LoggingInterceptor;
}

class Radapter::LoggingInterceptor : public Radapter::InterceptorBase
{
    Q_OBJECT
public:
    explicit LoggingInterceptor(const LoggingInterceptorSettings &settings);
    const QString &baseFilepath() const {return m_settings.filepath;}
    QString currentFilepath() const {return m_file->fileName();}
    bool isFull() const;
public slots:
    virtual void onMsgFromWorker(const Radapter::WorkerMsg &msg) override;
private slots:
    void onFlush();
private:
    bool testMsgForLog(const Radapter::WorkerMsg &msg);
    void enqueueMsg(const WorkerMsg &msg);

    QFile *m_file;
    QTimer *m_flushTimer;
    LoggingInterceptorSettings m_settings;
    QJsonArray m_array;
    bool m_shouldUpdate{false};

    bool m_error;
};
#endif
