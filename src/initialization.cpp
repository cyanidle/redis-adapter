#include "initialization.h"
#include "broker/broker.h"
#include "broker/interceptor/interceptor.h"
#include "broker/workers/worker.h"
#include "plugins/radapter_plugin.h"
#include "validators/validator_fetch.h"
#include <QDir>
#include "templates/algorithms.hpp"
#include <QLibrary>

void initPipe(const QString& pipe)
{
    static auto isInterceptor = [](const QString &worker) {
        return worker.startsWith('|') && worker.endsWith('|');
    };
    auto broker = Radapter::Broker::instance();
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
    for (auto pair: Radapter::zip(workers, interceptors)) {
        auto sourceName = pair.first.first;
        auto targetName = pair.first.second;
        auto interceptorNames = pair.second;
        broker->connectTwo(sourceName, targetName, interceptorNames);
    }
}

void Radapter::initPipelines(const QStringList &pipelines)
{
    for (const auto &pipe: pipelines) {
        settingsParsingWarn() << "Initializing pipeline:" << pipe;
        initPipe(pipe);
    }
}

void initPlugin(::Radapter::Plugin *plugin)
{
    auto validators = plugin->validators();
    for (auto iter = validators.begin(); iter != validators.end(); ++iter) {
        for (const auto &alias: iter.value()) {
            settingsParsingWarn() << "Adding validator with name:" << alias << "from plugin:" << plugin->name();
        }
        Validator::makeFetchable(iter.key(), iter.value());
    }

}

void Radapter::initPlugins(const QList<QLibrary*> plugins)
{
    for (auto *lib: plugins) {
        auto loader = (PluginLoaderFunction) lib->resolve(PLUGIN_LOADER_FUNCTION_NAME);
        if (!loader) {
            throw std::runtime_error("Could not find loader function in: " +
                                     lib->fileName().toStdString() +
                                     "; Do not forget RADAPTER_DECLARE_PLUGIN(<plugin-class>) in .cpp file!");
        }
        auto plugin = loader();
        settingsParsingWarn().nospace() << "[" << lib->fileName() << "] Found plugin: " << plugin->name();
        initPlugin(plugin);
    }
}






