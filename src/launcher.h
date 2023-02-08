#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QCommandLineParser>
#include "settings/settings.h"
#include "settings-parsing/filereader.h"
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
public slots:
    void addWorker(Radapter::Worker *worker, QSet<Radapter::InterceptorBase *> interceptors = {});
    void init();
    void run();
private:
    void setLoggingFilters(const QMap<QString, bool> &loggers);
    void preInitFilters();

    void initLogging();
    void preInit();
    void prvInit();
    void initRedis();
    void initModbus();
    void initWebsockets();
    void initSql();
    void parseCommandlineArgs();

    template <typename T>
    QList<T> parseTomlArray(const QString &tomlPath);
    template <typename T>
    T parseTomlObj(const QString &tomlPath, bool mustHave = false);
    QVariant readToml(const QString &tomlPath = "");
    bool setTomlPath(const QString &tomlPath);
    QHash<Worker*, QSet<InterceptorBase*>> m_workers;
    Settings::FileReader* m_filereader;
    QString m_configsDir{"conf"};
    QCommandLineParser m_parser{};
};

template <typename T>
QList<T> Launcher::parseTomlArray(const QString &tomlPath) {
    auto result = readToml(tomlPath);
    if (result.isValid() && !result.canConvert<QVariantList>()) {
        throw std::runtime_error("Expected List from: " + tomlPath.toStdString());
    }
    try {
        return Serializer::fromQList<T>(result.toList());
    } catch (const std::exception &e) {
        throw std::runtime_error(QString(e.what()).replace(ROOT_PREFIX, tomlPath).toStdString() +
                                 std::string("; While Parsing List --> ") +
                                 tomlPath.toStdString() + "; In --> " +
                                 m_filereader->filePath().toStdString());
    }
}
template <typename T>
T Launcher::parseTomlObj(const QString &tomlPath, bool mustHave) {
    auto result = readToml(tomlPath);
    if (result.isValid() && !result.canConvert<QVariantMap>()) {
        throw std::runtime_error("Expected Map from: " + tomlPath.toStdString());
    } else if ((!result.isValid() || !result.canConvert<QVariantMap>()) && !mustHave) {
        return {};
    }
    try {
        return Serializer::fromQMap<T>(result.toMap());
    } catch (const std::exception &e) {
        throw std::runtime_error(QString(e.what()).replace(ROOT_PREFIX, tomlPath).toStdString() +
                                 std::string("; While Parsing Object --> ") +
                                 tomlPath.toStdString() + "; In --> " +
                                 m_filereader->filePath().toStdString());
    }
}

}

#endif // LAUNCHER_H
