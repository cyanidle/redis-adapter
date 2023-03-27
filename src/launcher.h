#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QCommandLineParser>
#include "settings/settings.h"
#include "settings-parsing/dictreader.h"
#include <QObject>

namespace Radapter {
class Worker;
class InterceptorBase;
class Broker;
class RADAPTER_API Launcher : public QObject
{
    Q_OBJECT
public:
    explicit Launcher(QObject *parent = nullptr);
    const QString &configsDirectory() const;
    QCommandLineParser &commandLineParser();
    template<typename NewWorker, typename NewWorkerSettings>
    NewWorker* addWorker(const QString &key, QSet<Radapter::InterceptorBase *> interceptors = {});
signals:
    void started();
public slots:
    void addWorker(Radapter::Worker *worker, QSet<Radapter::InterceptorBase *> interceptors = {});
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

    void initRedis();
    void initModbus();
    void initWebsockets();
    void initSockets();
    void initSql();
    void initGui();
    void initPipelines();
    void initMocks();
    void initLocalization();
    void parseCommandlineArgs();
    void initProxies();


    Broker* broker() const;
    template <typename T>
    const QList<T> parseArrayOf(const QString &path = "");
    template <typename T>
    T parseObject(const QString &path = "");
    QVariant readSetting(const QString &path = "");
    QHash<Worker*, QSet<InterceptorBase*>> m_workers;
    Settings::Reader* m_reader;
    QString m_configsResource{"conf"};
    QString m_configsFormat{"toml"};
    QString m_mainPath{"config"};
    QVariantMap m_allSettings{};
    QCommandLineParser m_argsParser{};
};

template<typename NewWorker, typename NewWorkerSettings>
NewWorker* Launcher::addWorker(const QString &key, QSet<InterceptorBase *> interceptors)
{
    auto fromConfig = parseObject<NewWorkerSettings>(key);
    auto newWorker = new NewWorker(fromConfig, newThread());
    addWorker(newWorker, interceptors);
    return newWorker;
}

template <typename T>
const QList<T> Launcher::parseArrayOf(const QString &path) {
    auto result = readSetting(path);
    if (result.isValid() && !result.canConvert<QVariantList>()) {
        throw std::runtime_error("Expected List from: " + path.toStdString());
    }
    if (!result.canConvert<QVariantList>()) {
        settingsParsingWarn() << "Empty list for:"  << "[[" << path << "]]!";
    }
    try {
        return Serializable::fromQList<T>(result.toList());
    } catch (const std::exception &e) {
        throw std::runtime_error(QString(e.what()).toStdString() +
                                 std::string("; While Parsing List --> [[") +
                                 path.toStdString() + "]]; In --> " +
                                 m_reader->path().toStdString());
    }
}
template <typename T>
T Launcher::parseObject(const QString &path) {
    auto result = readSetting(path);
    if (result.isValid() && !result.canConvert<QVariantMap>()) {
        throw std::runtime_error("Expected Map from: " + path.toStdString());
    }
    try {
        return Serializable::fromQMap<T>(result.toMap());
    } catch (const std::exception &e) {
        throw std::runtime_error(QString(e.what()).toStdString() +
                                 std::string("; While Parsing Object --> [") +
                                 path.toStdString() + "]; In --> " +
                                 m_reader->path().toStdString());
    }
}

}

#endif // LAUNCHER_H
