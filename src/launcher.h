#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QCommandLineParser>
#include "settings/settings.h"
#include "settings-parsing/dictreader.h"
#include "broker/worker/worker.h"
#include <QObject>

namespace Radapter {

class RADAPTER_SHARED_SRC Launcher : public QObject
{
    Q_OBJECT
public:
    explicit Launcher(QObject *parent = nullptr);
    const QString &configsDirectory() const;
    QCommandLineParser &commandLineParser();
signals:
    void started();
public slots:
    void addWorker(Radapter::Worker *worker, QSet<Radapter::InterceptorBase *> interceptors = {});
    void init();
    void run();
private:
    void setLoggingFilters(const QMap<QString, bool> &loggers);
    void preInitFilters();

    void initLogging();
    void preInit();
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


    Broker* broker() const;
    template <typename T>
    QList<T> parseArray(const QString &tomlPath = "");
    template <typename T>
    T parseObj(const QString &tomlPath = "", bool mustHave = false);
    QVariant readSettings(const QString &tomlPath = "");
    void setSettingsPath(const QString &tomlPath);
    QHash<Worker*, QSet<InterceptorBase*>> m_workers;
    Settings::Reader* m_reader;
    QString m_configsDir{"conf"};
    QString m_configsFormat{"toml"};
    QCommandLineParser m_parser{};
};

template <typename T>
QList<T> Launcher::parseArray(const QString &tomlPath) {
    auto result = readSettings(tomlPath);
    if (result.isValid() && !result.canConvert<QVariantList>()) {
        throw std::runtime_error("Expected List from: " + tomlPath.toStdString());
    }
    try {
        return Serializer::fromQList<T>(result.toList());
    } catch (const std::exception &e) {
        throw std::runtime_error(QString(e.what()).replace(ROOT_PREFIX, tomlPath).toStdString() +
                                 std::string("; While Parsing List --> [[") +
                                 tomlPath.toStdString() + "]]; In --> " +
                                 m_reader->path().toStdString());
    }
}
template <typename T>
T Launcher::parseObj(const QString &tomlPath, bool mustHave) {
    auto result = readSettings(tomlPath);
    if (result.isValid() && !result.canConvert<QVariantMap>()) {
        throw std::runtime_error("Expected Map from: " + tomlPath.toStdString());
    } else if ((!result.isValid() || !result.canConvert<QVariantMap>()) && !mustHave) {
        return {};
    }
    try {
        return Serializer::fromQMap<T>(result.toMap());
    } catch (const std::exception &e) {
        throw std::runtime_error(QString(e.what()).replace(ROOT_PREFIX, tomlPath).toStdString() +
                                 std::string("; While Parsing Object --> [") +
                                 tomlPath.toStdString() + "]; In --> " +
                                 m_reader->path().toStdString());
    }
}

}

#endif // LAUNCHER_H
