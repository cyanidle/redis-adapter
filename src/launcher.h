#ifndef LAUNCHER_H
#define LAUNCHER_H

#include <QObject>
#include "settings-parsing/reader.h"

class QCommandLineParser;
namespace Settings {
struct AppConfig;
class Reader;
}
namespace Radapter {
class Worker;
class Interceptor;
class Broker;
struct LauncherPrivate;
class RADAPTER_API Launcher : public QObject
{
    Q_OBJECT
    struct Private;
public:
    explicit Launcher(QObject *parent = nullptr);
    const QString &configsDirectory() const;
    const Settings::AppConfig &config() const;
    Settings::Reader *reader();
    QCommandLineParser &commandLineParser();
    //! run() starts all configured radapter modules and workers
    void run();
    //! exec() calls run() and then return with QCoreApplication::exec();
    int exec();
    ~Launcher();
signals:
    void started();
public slots:
    void addWorker(Radapter::Worker *worker);
    void addInterceptor(const QString &name, Radapter::Interceptor *interceptor);
private:
    QThread *newThread();
    Broker* broker() const;

    void parseCommandlineArgs();
    void initConfig();
    void initPlugins();

    Private *d;
};

}

#endif // LAUNCHER_H
