#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QCommandLineParser>
#include "settings/settings.h"
#include "settings-parsing/dictreader.h"
#include "broker/worker/worker.h"
#include <QObject>

#include "settings-parsing/experimental/serializable.h"



struct Test : Serializable::Gadget {
    Q_GADGET
    FIELDS(a, b)
    Serializable::Field<int> a;
    Serializable::Field<int> b;
};

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

    void initLogging();
    void appPreInit();
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

template <typename T>
const QList<T> Launcher::parseArrayOf(const QString &path) {
    auto result = readSetting(path);
    if (result.isValid() && !result.canConvert<QVariantList>()) {
        throw std::runtime_error("Expected List from: " + path.toStdString());
    }
    try {
        return Serializer::fromQList<T>(result.toList());
    } catch (const std::exception &e) {
        throw std::runtime_error(QString(e.what()).replace(ROOT_PREFIX, path).toStdString() +
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
    } else if (!result.isValid() || !result.canConvert<QVariantMap>()) {
        return {};
    }
    try {
        return Serializer::fromQMap<T>(result.toMap());
    } catch (const std::exception &e) {
        throw std::runtime_error(QString(e.what()).replace(ROOT_PREFIX, path).toStdString() +
                                 std::string("; While Parsing Object --> [") +
                                 path.toStdString() + "]; In --> " +
                                 m_reader->path().toStdString());
    }
}

}

#endif // LAUNCHER_H
