#include "initialization.h"
#include "broker/broker.h"
#include "broker/interceptor/interceptor.h"
#include "broker/workers/worker.h"
#include "plugins/radapter_plugin.h"
#include "validators/validator_fetch.h"
#include <QDir>
#include <QLibrary>

void Radapter::initPipelines(const QStringList &pipelines)
{
    static auto isInterceptor = [](const QString &worker) {
        return worker.startsWith('|') && worker.endsWith('|');
    };
    static QRegExp splitter("[\\<\\>]");
    auto broker = Broker::instance();
    for (const auto &pipe: pipelines) {
        auto split = pipe.split(splitter);
        if (split.size() < 2) {
            throw std::runtime_error("Pipeline length must be more than 2!");
        }
        auto currentPos = 0;
        auto lastWorker = split.takeFirst().simplified();
        if (isInterceptor(lastWorker)) {
            throw std::runtime_error("Pipeline cannot begin with interceptor: " + lastWorker.toStdString());
        }
        for (const auto &worker : split) {
            currentPos = splitter.indexIn(pipe, currentPos);
            auto simplified = worker.simplified();
            if (isInterceptor(simplified)) {
                auto workerPtr = broker->getWorker(lastWorker);
                if (!workerPtr) {
                    throw std::runtime_error("Could not fetch worker with name: " + lastWorker.toStdString());
                }
                auto interName = simplified.replace('|', "").simplified();
                auto interceptor = broker->getInterceptor(interName);
                if (!interceptor) {
                    throw std::runtime_error("Could not fetch interceptor with name: " + interName.toStdString());
                }
                workerPtr->addInterceptor(interceptor);
                continue;
            }
            auto op = pipe[currentPos];
            if (op == "<") {
                broker->connectTwo(simplified, lastWorker);
            } else if (op == ">") {
                broker->connectTwo(lastWorker, simplified);
            }
            lastWorker = worker.simplified();
        }
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






