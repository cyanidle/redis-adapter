#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QCommandLineParser>
#include "settings/settings.h"
#include "settings-parsing/dictreader.h"
#include <QObject>

namespace Radapter {
class Worker;
class Interceptor;
class Broker;
class RADAPTER_API Launcher : public QObject
{
    Q_OBJECT
public:
    explicit Launcher(QObject *parent = nullptr);
    const QString &configsDirectory() const;
    Settings::Reader *reader();
    QCommandLineParser &commandLineParser();
    template<typename NewWorker, typename NewWorkerSettings>
    NewWorker* addWorkerWithConfig(const QString &key, QSet<Radapter::Interceptor *> interceptors = {});
signals:
    void started();
public slots:
    void addWorker(Radapter::Worker *worker, QSet<Radapter::Interceptor *> interceptors = {});
    //! run() starts all configured radapter modules and workers
    void run();
    //! exec() calls run() and then return with QCoreApplication::exec();
    int exec();
private:
    QThread *newThread();

    void setLoggingFilters(const QMap<QString, bool> &loggers);

    void initLogging();
    void initRoutedJsons();
    void initFilters();
    void initValidators();

    void initRedis();
    void initModbus();
    void initWebsockets();
    void initSockets();
    void initSql();
    void initPipelines();
    void initMocks();
    void initPlugins();
    void initLocalization();
    void initLoggingWorkers();
    void preInit();
    void initBrokerSettings();
    void parseCommandlineArgs();
    void initProxies();

    Broker* broker() const;
    template <typename T> T parseObject(const QString &path = "");
    template <typename T> const QList<T> parseArrayOf(const QString &path = "");
    template <typename T> const QMap<QString, T> parseMapOf(const QString &path = "");
    QVariant readSetting(const QString &path = "");
    QHash<Worker*, QSet<Interceptor*>> m_workers;
    Settings::Reader* m_reader;
    QString m_configsResource;
    QString m_configsFormat;
    QString m_file;
    QString m_pluginsPath;
    QVariantMap m_allSettings{};
    QCommandLineParser m_argsParser{};
};

template<typename NewWorker, typename NewWorkerSettings>
NewWorker* Launcher::addWorkerWithConfig(const QString &key, QSet<Interceptor *> interceptors)
{
    auto fromConfig = parseObject<NewWorkerSettings>(key);
    auto newWorker = new NewWorker(fromConfig, newThread());
    addWorker(newWorker, interceptors);
    return newWorker;
}

}

#endif // LAUNCHER_H
