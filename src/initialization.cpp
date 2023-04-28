#include "initialization.h"
#include "broker/broker.h"
#include "broker/interceptor/interceptor.h"
#include "broker/workers/mockworker.h"
#include "broker/workers/repeaterworker.h"
#include "broker/workers/settings/mockworkersettings.h"
#include "broker/workers/settings/repeatersettings.h"
#include "broker/workers/worker.h"
#include "interceptors/namespaceunwrapper.h"
#include "interceptors/namespacewrapper.h"
#include "interceptors/settings/namespaceunwrappersettings.h"
#include "interceptors/settings/namespacewrappersettings.h"
#include "interceptors/settings/validatinginterceptorsettings.h"
#include "interceptors/validatinginterceptor.h"
#include "launcher.h"
#include "qthread.h"
#include "validators/validator_fetch.h"
#include <QDir>
#include "templates/algorithms.hpp"
#include <QLibrary>

using namespace Radapter;

struct FuncResult {
    QString func;
    QStringList data;
};

FuncResult parseFunc(const QString &rawFunc)
{
    static auto splitter = QRegularExpression("\\(.*\\)");
    auto func = rawFunc.split(splitter)[0];
    auto data = splitter
                    .match(rawFunc) // func(data, data)
                    .captured().remove(0, 1).chopped(1) // data, data
                    .split(','); // [data, data]
    for (auto &item: data){
        item = item.simplified();
    };
    return {func, data};
}

void tryCreateInterceptor(const QString &name, QObject *parent)
{
    Q_UNUSED(parent)
    auto broker = Radapter::Broker::instance();
    auto [func, data] = parseFunc(name);
    if (func == "wrap") {
        auto config = Settings::NamespaceWrapper();
        config.wrap_into = data[0];
        broker->registerInterceptor(name, new NamespaceWrapper(config));
    } else if (func == "unwrap") {
        auto config = Settings::NamespaceUnwrapper();
        config.unwrap_from = data[0];
        broker->registerInterceptor(name, new NamespaceUnwrapper(config));
    } else if (func == "add_timestamp") {
        auto config = Settings::ValidatingInterceptor();
        config.by_field.value = {{data[0], "set_unix_timestamp"}};
        broker->registerInterceptor(name, new ValidatingInterceptor(config));
    } else {
        throw std::runtime_error("(" + name.toStdString() + ") is not supported in pipe!");
    }
}

void tryCreateWorker(const QString &name, QObject *parent)
{
    auto broker = Radapter::Broker::instance();
    auto [func, _] = parseFunc(name);
    if (func == "repeater") {
        auto config = Settings::Repeater();
        config.name = name;
        broker->registerWorker(new Repeater(config, new QThread(parent)));
    } else if (func == "mock") {
        auto config = Settings::MockWorker();
        config.name = name;
        broker->registerWorker(new MockWorker(config, new QThread(parent)));
    } else {
        throw std::runtime_error("(" + name.toStdString() + ") is not supported in pipe!");
    }
}

void tryConnecting(const QString &producer, const QString &consumer, const QStringList &pipe, QObject *parent)
{
    auto broker = Radapter::Broker::instance();
    for (auto &step: pipe) {
        auto existing = broker->getInterceptor(step);
        if (!existing) {
            tryCreateInterceptor(step, parent);
        }
    }
    auto existingProd = broker->getWorker(producer);
    if (!existingProd) {
        tryCreateWorker(producer, parent);
    }
    auto existingCons = broker->getWorker(consumer);
    if (!existingCons) {
        tryCreateWorker(consumer, parent);
    }
    broker->connectTwo(producer, consumer, pipe);
}

void Radapter::initPipe(const QString& pipe, QObject *parent)
{
    static auto isInterceptor = [](const QString &worker) {
        return worker.startsWith('|') && worker.endsWith('|');
    };
    auto split = pipe.split('>');
    if (split.size() < 2) {
        throw std::runtime_error("Pipeline length must be more than 2!");
    }
    auto prevWorker = split.takeFirst().simplified();
    auto lastWorker = split.constLast().simplified();
    if (isInterceptor(prevWorker)) {
        throw std::runtime_error("Pipeline cannot begin with an interceptor: " + prevWorker.toStdString());
    }
    if (isInterceptor(lastWorker)) {
        throw std::runtime_error("Pipeline cannot end with an interceptor: " + lastWorker.toStdString());
    }
    QList<QPair<QString, QString>> workers;
    QList<QStringList> interceptors;
    QStringList currentInterceptors;
    for (const auto &point : split) {
        auto current = point.simplified();
        if (isInterceptor(current)) {
            currentInterceptors.append(current.replace('|', "").simplified());
            continue;
        }
        workers.append({prevWorker, current});
        interceptors.append(currentInterceptors);
        currentInterceptors.clear();
        prevWorker = point.simplified();
    }
    for (auto [pair, interceptorNames]: Radapter::zip(workers, interceptors)) {
        auto [sourceName, targetName] = pair;
        tryConnecting(sourceName, targetName, interceptorNames, parent);
    }
}

void Radapter::initPipelines(const QStringList &pipelines, QObject *parent)
{
    for (const auto &pipe: pipelines) {
        settingsParsingWarn() << "Initializing pipeline:" << pipe;
        initPipe(pipe, parent);
    }
}






