#include <QLibrary>
#include <QCommandLineParser>
#include "broker/broker.h"
#include "consumers/rediscacheconsumer.h"
#include "consumers/rediskeyeventsconsumer.h"
#include "consumers/redisstreamconsumer.h"
#include "httpserver/radapterapi.h"
#include "initialization.h"
#include "interceptors/duplicatinginterceptor.h"
#include "interceptors/validatinginterceptor.h"
#include "modbus/modbusmaster.h"
#include "modbus/modbusslave.h"
#include "producers/rediscacheproducer.h"
#include "producers/redisstreamproducer.h"
#include "validators/common_validators.h"
#include "launcher.h"
#include "radapterlogging.h"
#include "localization.h"
#include "localstorage.h"
#include "routed_object/routesprovider.h"
#include "settings-parsing/adapters/yaml.hpp"
#include "broker/workers/mockworker.h"
#include "broker/workers/loggingworker.h"
#include "filters/producerfilter.h"
#include "radapterconfig.h"
#ifdef Q_OS_UNIX
#include "utils/resourcemonitor.h"
#endif
using namespace Radapter;

struct Radapter::LauncherPrivate {
    Settings::Reader* reader;
    QString configsResource;
    QString configsFormat;
    QString file;
    QString pluginsPath;
    QString configKey;
    QCommandLineParser argsParser;
    Settings::AppConfig config;
};

Launcher::Launcher(QObject *parent) :
    QObject(parent),
    d(new LauncherPrivate)
{
    qSetMessagePattern(RADAPTER_CUSTOM_MESSAGE_PATTERN);
    d->reader = nullptr;
    d->argsParser.setApplicationDescription(
        "Redis Adapter. System for routing and collecting information from and to different protocols/devices/code modules.");
    parseCommandlineArgs();
    Validator::registerAllCommon();
    initPlugins();
    initConfig();
}

void Launcher::initConfig()
{
    auto configMap = reader()->get(d->configKey).toMap();
    d->config.allowExtra();
    d->config.update(configMap);
    for (const auto& config: d->config.redis->cache->consumers) {
        addWorker(new Redis::CacheConsumer(config, newThread()));
    }
    for (const auto& config: d->config.redis->stream->consumers) {
        addWorker(new Redis::StreamConsumer(config, newThread()));
    }
    for (const auto& config: d->config.sockets->udp->consumers) {
        addWorker(new Udp::Consumer(config, newThread()));
    }
    for (const auto& config: d->config.redis->cache->producers) {
        addWorker(new Redis::CacheProducer(config, newThread()));
    }
    for (const auto &config: d->config.redis->key_events->subscribers) {
        addWorker(new Redis::KeyEventsConsumer(config, newThread()));
    }
    for (const auto& config: d->config.redis->stream->producers) {
        addWorker(new Redis::StreamProducer(config, newThread()));
    }
    for (const auto& config: d->config.sockets->udp->producers) {
        addWorker(new Udp::Producer(config, newThread()));
    }
    for (const auto& config: d->config.sockets->udp->consumers) {
        addWorker(new Udp::Consumer(config, newThread()));
    }
    for (const auto& config: d->config.mocks) {
        addWorker(new MockWorker(config, newThread()));
    }
    for (const auto& config: d->config.logging_workers) {
        addWorker(new LoggingWorker(config, newThread()));
    }
    for (const auto& config: d->config.modbus->slaves) {
        addWorker(new Modbus::Slave(config, newThread()));
    }
    for (const auto& config: d->config.modbus->masters) {
        addWorker(new Modbus::Master(config, newThread()));
    }
    for (auto iter = d->config.interceptors->duplicating->begin(); iter != d->config.interceptors->duplicating->end(); ++iter) {
        addInterceptor(iter.key(), new DuplicatingInterceptor(iter.value()));
    }
    for (auto iter = d->config.interceptors->validating->begin(); iter != d->config.interceptors->validating->end(); ++iter) {
        addInterceptor(iter.key(), new ValidatingInterceptor(iter.value()));
    }
    if (d->config.api->enabled) {
        addWorker(new ApiServer(d->config.api, newThread(), this));
    }
    LocalStorage::init(this);
}

void Launcher::initPlugins()
{
    QDir dir(d->pluginsPath);
    if (!dir.exists()) {
        settingsParsingWarn() << "No plugins dir: " << dir.absolutePath();
        return;
    }
    settingsParsingWarn() << "Seaching dir for plugins:" << dir.absolutePath();
    QList<QLibrary*> plugins;
    for (const auto &file: dir.entryList(QDir::Filter::Files | QDir::Filter::NoSymLinks)) {
        auto path = dir.filePath(file);
        if (!QLibrary::isLibrary(path)) {
            settingsParsingWarn() << path << "is not a library. Skipping";
            continue;
        }
        auto lib = new QLibrary(path, this);
        plugins.append(lib);
    }
    settingsParsingWarn() << "Found" << plugins.size() << "plugin(s)!";
    Radapter::initPlugins(plugins);
}

void Launcher::parseCommandlineArgs()
{
    if (QCoreApplication::applicationName().isEmpty()) {
        QCoreApplication::setApplicationName("redis-adapter");
    }
    if (QCoreApplication::applicationVersion().isEmpty()) {
        QCoreApplication::setApplicationVersion(RADAPTER_VERSION);
    }
    d->argsParser.addHelpOption();
    d->argsParser.addVersionOption();
    d->argsParser.addOptions({
                  {{"d", "directory"},
                    "Config will be read from <directory>. (default: ./conf)", "directory", "conf"},
                  {{"p", "parser"},
                    "Config will be read from .<parser_format> files. (available: yaml) (default: yaml)", "parser", "yaml"},
                  {{"f", "file"},
                    "File to read settings from. (default: <directory>/config.<parser-extension>)", "file", "config"},
                  {{QString("plugins-dir")},
                    "Directory with plugins. (default: ./plugins)", "plugins-dir", "plugins"},
                  {{"c", "config-key"},
                   "Subkey of Settings::AppConfig. (default: ''(root))", "config-key", ""},
                  });
    d->argsParser.process(*QCoreApplication::instance());
    d->configsResource = d->argsParser.value("directory");
    d->configsFormat = d->argsParser.value("parser");
    d->pluginsPath = d->argsParser.value("plugins-dir");
    d->file = d->argsParser.value("file");
    d->configKey = d->argsParser.value("config-key");
    if (d->configsFormat == "yaml") {
        d->reader = new Settings::YamlReader(d->configsResource, d->file, this);
    } else {
        throw std::runtime_error("Invalid configs format: " + d->configsFormat.toStdString() + "; Can be (toml/yaml)");
    }
    settingsParsingWarn() << "Using parser:" << d->configsFormat;
}

Broker *Launcher::broker() const
{
    return Broker::instance();
}


void Launcher::addWorker(Radapter::Worker *worker)
{
    broker()->registerWorker(worker);
}

void Launcher::addInterceptor(const QString& name, Interceptor *interceptor)
{
    broker()->registerInterceptor(name, interceptor);
}

void Launcher::run()
{
#ifdef Q_OS_UNIX
    auto resmonThr = new QThread(this);
    auto resmon = new ResourceMonitor();
    connect(resmonThr, &QThread::started, resmon, &ResourceMonitor::run);
    resmon->moveToThread(resmonThr);
    resmonThr->start(QThread::LowPriority);
#endif
    initPipelines(d->config.pipelines.value);
    Broker::instance()->runAll();
    emit started();
}

int Launcher::exec()
{
    run();
    return QCoreApplication::instance()->exec();
}

QThread *Launcher::newThread()
{
    return new QThread(this);
}

QCommandLineParser &Launcher::commandLineParser()
{
    return d->argsParser;
}

Launcher::~Launcher()
{
    delete d;
}

const QString &Launcher::configsDirectory() const
{
    return d->configsResource;
}

const Settings::AppConfig &Launcher::config() const
{
    return d->config;
}

Settings::Reader *Launcher::reader()
{
    return d->reader;
}

