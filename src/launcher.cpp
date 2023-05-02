#include <QLibrary>
#include <QCommandLineParser>
#include "broker/broker.h"
#include "broker/workers/repeaterworker.h"
#include "consumers/rediscacheconsumer.h"
#include "consumers/rediskeyeventsconsumer.h"
#include "consumers/redisstreamconsumer.h"
#include "httpserver/radapterapi.h"
#include "initialization.h"
#include "interceptors/duplicatinginterceptor.h"
#include "interceptors/namespaceunwrapper.h"
#include "interceptors/namespacewrapper.h"
#include "interceptors/validatinginterceptor.h"
#include "modbus/modbusmaster.h"
#include "radapterlogging.h"
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
#include "broker/workers/fileworker.h"
#include "filters/producerfilter.h"
#include "radapterconfig.h"
#include "websocket/websocketclient.h"
#include "websocket/websocketserver.h"
#ifdef Q_OS_UNIX
#include "utils/resourcemonitor.h"
#endif
using namespace Radapter;

struct Radapter::Launcher::Private {
    Settings::Reader* reader;
    QString configsResource;
    QString configsFormat;
    QString file;
    //QString pluginsPath;
    QString configKey;
    QCommandLineParser argsParser;
    Settings::AppConfig config;
    bool readConfig{true};
    QStringList configOverrides;
};

Launcher::Launcher(QObject *parent) :
    QObject(parent),
    d(new Private)
{
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]{
        delete this;
    });
    qSetMessagePattern(RADAPTER_CUSTOM_MESSAGE_PATTERN);
    d->reader = nullptr;
    d->argsParser.setApplicationDescription(
        "Redis Adapter. System for routing and collecting information from and to different protocols/devices/code modules.");
    parseCommandlineArgs();
    Validator::registerAllCommon();
    initConfig();
}

void Launcher::initConfig()
{
    JsonDict configMap;
    if (d->readConfig) {
        configMap = JsonDict(reader()->get(d->configKey).toMap(), false);
    }
    for (auto &config: d->configOverrides) {
        assert(config.contains('='));
        auto split = config.split('=');
        auto key = split[0];
        auto val = split[1];
        settingsParsingWarn() << "Overriding config value for:" << key << "with" << val;
        configMap[key] = val;
    }
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
    for (const auto& config: d->config.files) {
        addWorker(new FileWorker(config, newThread()));
    }
    for (const auto& config: d->config.modbus->slaves) {
        addWorker(new Modbus::Slave(config, newThread()));
    }
    for (const auto& config: d->config.modbus->masters) {
        addWorker(new Modbus::Master(config, newThread()));
    }
    for (const auto& config: d->config.websocket->servers) {
        addWorker(new Websocket::Server(config, newThread()));
    }
    for (const auto& config: d->config.websocket->clients) {
        addWorker(new Websocket::Client(config, newThread()));
    }
    for (const auto& config: d->config.repeaters) {
        addWorker(new Repeater(config, newThread()));
    }
    for (auto [name, config]: d->config.interceptors->duplicating) {
        addInterceptor(name, new DuplicatingInterceptor(config));
    }
    for (auto [name, config]: d->config.interceptors->validating) {
        addInterceptor(name, new ValidatingInterceptor(config));
    }
    for (auto [name, config]: d->config.interceptors->namespaces->unwrappers) {
        addInterceptor(name, new NamespaceUnwrapper(config));
    }
    for (auto [name, config]: d->config.interceptors->namespaces->wrappers) {
        addInterceptor(name, new NamespaceWrapper(config));
    }
    if (d->config.api->enabled) {
        addWorker(new ApiServer(d->config.api, newThread(), this));
    }
    LocalStorage::init(this);
}
void Launcher::parseCommandlineArgs()
{
    if (QCoreApplication::applicationName().isEmpty()) {
        QCoreApplication::setApplicationName("redis-adapter");
    }
    if (QCoreApplication::applicationVersion().isEmpty()) {
        QCoreApplication::setApplicationVersion(RADAPTER_VERSION);
    }
    d->argsParser.addOptions({
                  {{"d", "directory"},
                    "Config will be read from <directory> (default: $(pwd)/conf).", "directory", "conf"},
                  {{"p", "parser"},
                    "Config will be read from <.parser_format> files (available: yaml) (default: yaml).", "parser", "yaml"},
                  {{"f", "file"},
                    "File to read settings from (default: <directory>/config.<parser-extension>).", "file", "config"},
                  {{"c", "config"},
                    "Add/Override config value (use ':' for nesting, '=' for assignment). Example: api:enabled=false", "config", ""},
                  //{{QString("plugins-dir")},
                  //  "Directory with plugins (default: $(pwd)/plugins).", "plugins-dir", "plugins"},
                  {QString{"disable-config-reader"},
                   "Do not try reading config from non cli-args sources."},
                  {QString{"config-key"},
                   "Subkey to parse, for example a key in yaml file (default: '' --> (root)).", "config-key", ""},
                  {QString{"dump-config-example"},
                   "Write config example to stdout."},
                  });
    d->argsParser.addHelpOption();
    d->argsParser.addVersionOption();
    d->argsParser.process(*QCoreApplication::instance());
    d->configsResource = d->argsParser.value("directory");
    d->configsFormat = d->argsParser.value("parser");
    d->configOverrides = d->argsParser.values("config");
    //d->pluginsPath = d->argsParser.value("plugins-dir");
    d->file = d->argsParser.value("file");
    d->configKey = d->argsParser.value("config-key");
    auto isExamplesMode = d->argsParser.isSet("dump-config-example");
    d->readConfig = !d->argsParser.isSet("disable-config-reader");
    if (d->configsFormat == "yaml") {
        d->reader = new Settings::YamlReader(d->configsResource, d->file, this);
    } else {
        throw std::runtime_error("Invalid configs format: " + d->configsFormat.toStdString() + "; Can be (toml/yaml)");
    }
    if (isExamplesMode) {
        QTextStream(stdout) << d->reader->formatExample(config().getExample());
        std::exit(0);
    } else {
        settingsParsingWarn() << "Using parser:" << d->configsFormat;
    }
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
    connect(resmonThr, &QThread::destroyed, resmon, &QObject::deleteLater);
    resmon->moveToThread(resmonThr);
    resmonThr->start(QThread::LowPriority);
#endif
    for (const auto &pipe: d->argsParser.positionalArguments()) {
        initPipeline(pipe, this);
    }
    for (const auto &pipe: d->config.pipelines.value) {
        initPipeline(pipe, this);
    }
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

void Launcher::createPipe(const QString &pipe)
{
    Radapter::initPipeline(pipe, this);
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

