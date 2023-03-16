#include "broker/broker.h"
#include "broker/worker/mockworker.h"
#include "connectors/websocketserverconnector.h"
#include "guisettings.h"
#include "launcher.h"
#include "radapterlogging.h"
#include "localstorage.h"
#include "localization.h"
#include "bindings/bindingsprovider.h"
#include "consumers/redisstreamconsumer.h"
#include "consumers/rediscacheconsumer.h"
#include "consumers/rediskeyeventsconsumer.h"
#include "producers/sqlarchiveproducer.h"
#include "producers/redisstreamproducer.h"
#include "producers/rediscacheproducer.h"
#include "settings/modbussettings.h"
#include "settings/redissettings.h"
#include "modbus/modbusmaster.h"
#include "modbus/modbusslave.h"
#include "websocket/websocketclient.h"
#include "raw_sockets/udpproducer.h"
#include "raw_sockets/udpconsumer.h"
#include "settings-parsing/adapters/toml.hpp"
#include "settings-parsing/adapters/yaml.hpp"
#include "filters/producerfilter.h"
#ifdef Q_OS_UNIX
#include "utils/resourcemonitor.h"
#endif
#ifdef RADAPTER_GUI
#include "mainwindow.h"
#endif
using namespace Radapter;

Launcher::Launcher(QObject *parent) :
    QObject(parent),
    m_workers(),
    m_reader()
{
    qSetMessagePattern(RADAPTER_CUSTOM_MESSAGE_PATTERN);
    m_argsParser.setApplicationDescription("Redis Adapter");
    parseCommandlineArgs();
    auto tryInit = [&](void(Launcher::*method)(), const QString &moduleName) {
        try {
            (this->*method)();
        } catch(std::runtime_error &exc) {
            settingsParsingWarn() << "Could not enable module:" << moduleName << "Details:" << exc.what();
        }
    };
    tryInit(&Launcher::appPreInit, "pre-initialization");
    tryInit(&Launcher::initLogging, "logging");
    tryInit(&Launcher::initRedis, "redis");
    tryInit(&Launcher::initModbus, "modbus");
    tryInit(&Launcher::initWebsockets, "websockets");
    tryInit(&Launcher::initSql, "sql");
    tryInit(&Launcher::initSockets, "raw-sockets");
    tryInit(&Launcher::initMocks, "mocks");
    tryInit(&Launcher::initLocalization, "localization");
    LocalStorage::init(this);
}

void Launcher::addWorker(Worker* worker, QSet<InterceptorBase*> interceptors)
{
    m_workers.insert(worker, interceptors);
}

//! Все эти классы сохраняютс япри инициализации, логгинг их количества нужен,
//! чтобы компилятор не удалил "неиспользуемые переменные"
void Launcher::appPreInit()
{
    initFilters();
    auto jsonBindings = JsonBinding::parseMap(readSetting("bindings").toMap());
    BindingsProvider::init(jsonBindings);
    reDebug() << "config: Json Bindings count: " << jsonBindings.size();
    auto redisServers = parseArrayOf<Settings::RedisServer>("redis.servers");
    reDebug() << "config: RedisServer count: " << redisServers.size();
    auto sqlClientsInfo = parseArrayOf<Settings::SqlClientInfo>("sql.clients");
    reDebug() << "config: Sql clients count: " << sqlClientsInfo.size();
}


void Launcher::setLoggingFilters(const QMap<QString, bool> &loggers)
{
    auto filterRules = QStringList{
        "*.debug=false",
        "workers=true",
        "redis-adapter=true",
        "modbus=true",
        "mysql=true",
        "ws-server=true",
        "ws-client=true",
        "res-monitor=true",
        "settings-parsing=true",
        "broker=true",
        "json-bindings=true",
        "workers=true",
    };
    for (auto logInfo = loggers.begin(); logInfo != loggers.end(); logInfo++) {
        auto rule = QString("%1=%2").arg(logInfo.key(), logInfo.value() ? "true" : "false");
        if (filterRules.contains(rule)) {
            filterRules[filterRules.indexOf(rule)] = rule;
        } else {
            filterRules.append(rule);
        }
    }
    QMap<QString, QMap<QtMsgType, bool>> workerLoggers{};
    for (auto iter{loggers.begin()}; iter != loggers.end(); ++iter) {
        auto key = iter.key();
        auto val = iter.value();
        QtMsgType currentType = QtDebugMsg;
        if (key.endsWith(".debug")) {
            currentType = QtDebugMsg;
            key.remove(".debug");
        } else if (key.endsWith(".info")) {
            currentType = QtInfoMsg;
            key.remove(".info");
        } else if (key.endsWith(".warn")) {
            key.remove(".warn");
            currentType = QtWarningMsg;
        } else if (key.endsWith(".error")) {
            key.remove(".error");
            currentType = QtCriticalMsg;
        } else {
            auto &dbgEnabled = workerLoggers[key][QtDebugMsg];
            if (!dbgEnabled) {
                dbgEnabled = val;
            }
            auto &infoEnabled = workerLoggers[key][QtInfoMsg];
            if (!infoEnabled) {
                infoEnabled = val;
            }
            auto &warnEnabled = workerLoggers[key][QtWarningMsg];
            if (!warnEnabled) {
                warnEnabled = val;
            }
            auto &critEnabled = workerLoggers[key][QtCriticalMsg];
            if (!critEnabled) {
                critEnabled = val;
            }
            continue;
        }
        workerLoggers[key][currentType] = val;
    }
    Broker::instance()->applyWorkerLoggingFilters(workerLoggers);
    auto filterString = filterRules.join("\n");
    QLoggingCategory::setFilterRules(filterString);
}

void Launcher::initFilters()
{
    auto rawFilters = readSetting("filters").toMap();
    for (auto iter = rawFilters.constBegin(); iter != rawFilters.constEnd(); ++iter) {
        Settings::Filters::table().insert(iter.key(), convertQMap<double>(iter.value().toMap()));
    }
}

void Launcher::initLogging()
{
    auto rawMap = readSetting("log_debug").toMap();
    auto flattened = JsonDict{rawMap}.flatten(".");
    setLoggingFilters(convertQMap<bool>(flattened));
}

void Launcher::initGui()
{
#ifdef RADAPTER_GUI
    auto guiSettings = parseObject<GuiSettings>("gui");
    if (!guiSettings.enabled) {
        reWarn() << "Gui disabled!";
        return;
    }
    auto guiThread = new QThread(this);
    auto ui = new Gui::MainWindow();
    ui->moveToThread(guiThread);
    connect(this, &Launcher::started, this, [guiThread](){guiThread->start();});
#endif
}

void Launcher::initLocalization()
{
    auto localizationInfo = parseObject<Settings::LocalizationInfo>("localization");
    Localization::instance()->applyInfo(localizationInfo);
}

void Launcher::initMocks()
{
    for (const auto &mockSettings : parseArrayOf<Radapter::MockWorkerSettings>("mocks")) {
        addWorker(new Radapter::MockWorker(mockSettings, new QThread(this)));
    }
}

void Launcher::initPipelines()
{
    const auto parsed = parseObject<Settings::Pipelines>();
    static QRegExp splitter("[<>]");
    for (const auto &pipe: parsed.pipelines) {
        auto split = pipe.split(splitter);
        if (split.size() < 2) {
            throw std::runtime_error("Pipeline length must be more than 2!");
        }
        auto currentPos = 0;
        auto lastWorker = split.takeFirst().simplified();
        for (const auto &worker : split) {
            currentPos = splitter.indexIn(pipe, currentPos);
            auto op = pipe[currentPos];
            if (op == "<") {
                broker()->connectTwoProxies(worker.simplified(), lastWorker);
            } else if (op == ">") {
                broker()->connectTwoProxies(lastWorker, worker.simplified());
            }
            lastWorker = worker.simplified();
        }
    }
}

void Launcher::initSockets()
{
    for (const auto &udp : parseArrayOf<Udp::ProducerSettings>("sockets.udp.producers")) {
        addWorker(new Udp::Producer(udp, new QThread(this)));
    }
    for (const auto &udp : parseArrayOf<Udp::ConsumerSettings>("sockets.udp.consumers")) {
        addWorker(new Udp::Consumer(udp, new QThread(this)));
    }
}

void Launcher::initRedis()
{
    for (auto &streamConsumer : parseArrayOf<Settings::RedisStreamConsumer>("redis.stream.consumers")) {
        addWorker(new Redis::StreamConsumer(streamConsumer, new QThread(this)));
    }
    for (auto &streamProducer : parseArrayOf<Settings::RedisStreamProducer>("redis.stream.producers")) {
        addWorker(new Redis::StreamProducer(streamProducer, new QThread(this)));
    }
    for (auto &cacheConsumer : parseArrayOf<Settings::RedisCacheConsumer>("redis.cache.consumers")) {
        addWorker(new Redis::CacheConsumer(cacheConsumer, new QThread(this)));
    }
    for (auto &cacheProducer : parseArrayOf<Settings::RedisCacheProducer>("redis.cache.producers")) {
        addWorker(new Redis::CacheProducer(cacheProducer, new QThread(this)));
    }
    for (auto &keyEventConsumer : parseArrayOf<Settings::RedisKeyEventSubscriber>("redis.keyevents.subscribers")) {
        addWorker(new Redis::KeyEventsConsumer(keyEventConsumer, new QThread(this)));
    }
    //auto redisStreamGroupConsumers = readFromToml<Settings::RedisStreamGroupConsumer>("redis.stream.group_consumer");
}

void Launcher::initModbus()
{
    Settings::parseRegisters(readSetting("registers"));
    auto mbDevices = parseArrayOf<Settings::ModbusDevice>("modbus.devices");
    reDebug() << "config: Modbus devices count: " << mbDevices.size();
    for (const auto& slaveInfo : parseArrayOf<Settings::ModbusSlave>("modbus.slaves")) {
        addWorker(new Modbus::Slave(slaveInfo, new QThread(this)));
    }
    for (const auto& masterInfo : parseArrayOf<Settings::ModbusMaster>("modbus.masters")) {
        addWorker(new Modbus::Master(masterInfo, new QThread(this)));
    }
}

void Launcher::initWebsockets()
{
    for (auto &wsClient : parseArrayOf<Settings::WebsocketClientInfo>("websocket.clients")) {
        addWorker(new Websocket::Client(wsClient, new QThread(this)));
    }
    auto websocketServer = parseObject<Settings::WebsocketServerInfo>("websocket.server");
    if (websocketServer.isValid()) {
        addWorker(new Websocket::ServerConnector(websocketServer, new QThread(this)));
    }
}

void Launcher::initSql()
{
    for (auto &archive : parseArrayOf<Settings::SqlStorageInfo>("sql.storage.archives")) {
        addWorker(new MySql::ArchiveProducer(archive, new QThread(this)));
    }
}

void Launcher::parseCommandlineArgs()
{
    if (QCoreApplication::applicationName().isEmpty()) {
        QCoreApplication::setApplicationName("redis-adapter");
    }
    if (QCoreApplication::applicationVersion().isEmpty()) {
        QCoreApplication::setApplicationVersion(RADAPTER_VERSION);
    }
    m_argsParser.addHelpOption();
    m_argsParser.addVersionOption();
    m_argsParser.addOptions({
                  {{"d", "directory"},
                    "Config will be read from <directory> (default: ./conf)", "directory", "conf"},
                  {{"f", "format"},
                    "Config will be read from .<format> files (available: yaml, toml) (default: ./yaml)", "format", "yaml"},
                  {{"m", "main_file"},
                    "Allows to read config either from main file with <key.subkey> or from file <key> with <subkey> (default: config)", "main_file", "config"}
                  });
    m_argsParser.process(*QCoreApplication::instance());
    m_configsResource = m_argsParser.value("directory");
    m_configsFormat = m_argsParser.value("format");
    m_mainPath = m_argsParser.value("main_file");
    if (m_configsFormat == "toml") {
        m_reader = new Settings::TomlReader(m_configsResource, m_mainPath, this);
    } else if (m_configsFormat == "yaml") {
        m_reader = new Settings::YamlReader(m_configsResource, m_mainPath, this);
    } else {
        throw std::runtime_error("Invalid configs format: " + m_configsFormat.toStdString() + "; Can be (toml/yaml)");
    }
    settingsParsingWarn() << "Using parser:" << m_configsFormat;
}

Broker *Launcher::broker() const
{
    return Broker::instance();
}

QVariant Launcher::readSetting(const QString &path) {
    return m_reader->get(path);
}

void Launcher::init()
{
    for (auto workerIter = m_workers.begin(); workerIter != m_workers.end(); ++workerIter) {
        Broker::instance()->registerProxy(workerIter.key()->createProxy(workerIter.value()));
    }
}

void Launcher::run()
{
#ifdef Q_OS_UNIX
    auto resmonThr = new QThread(this);
    auto resmon = new ResourceMonitor(this);
    connect(resmonThr, &QThread::started, resmon, &ResourceMonitor::run);
    resmon->moveToThread(resmonThr);
    resmonThr->start(QThread::LowPriority);
#endif
    Broker::instance()->connectProducersAndConsumers();
    for (auto worker = m_workers.keyBegin(); worker != m_workers.keyEnd(); ++worker) {
        (*worker)->run();
    }
    Broker::instance()->runAll();
    initPipelines();
    emit started();
}

QCommandLineParser &Launcher::commandLineParser()
{
    return m_argsParser;
}

const QString &Launcher::configsDirectory() const
{
    return m_configsResource;
}

