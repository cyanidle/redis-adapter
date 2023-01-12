#ifndef LAUNCHER_H
#define LAUNCHER_H

#include "factories/mysqlfactory.h"
#include "settings-parsing/filereader.h"
#include "settings-parsing/serializer.hpp"
#include <QObject>
#include <radapter-broker/factorybase.h>

namespace Radapter {

class RADAPTER_SHARED_SRC Launcher : public QObject
{
    Q_OBJECT
public:
    explicit Launcher(QObject *parent = nullptr);
    MySqlFactory* mySqlFactory() const {return m_sqlFactory;}
public slots:
    void addFactory(Radapter::FactoryBase* factory);
    void addWorker(Radapter::WorkerBase *worker, QList<Radapter::InterceptorBase *> interceptors = {});

    int init();
    void run();
private:
    void setLoggingFilters(const Settings::LoggingInfo &loggers);
    int initWorkers();

    int prvInit();
    void prvPreInit();

    template <typename T>
    QList<T> precacheFromToml(const QString &tomlPath) {
        return Serializer::fromQList<T>(m_filereader->deserialise(tomlPath,true).toList());
    }
    QList<FactoryBase*> m_factories;
    QHash<WorkerBase*, QList<InterceptorBase*>> m_workers;
    MySqlFactory* m_sqlFactory;
    Settings::FileReader* m_filereader;
};
}

#endif // LAUNCHER_H
