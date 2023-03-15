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
#include "filters/producerfilter.h"
#include "producers/sqlarchiveproducer.h"
#include "producers/redisstreamproducer.h"
#include "producers/rediscacheproducer.h"
#include "settings/modbussettings.h"
#include "settings/redissettings.h"
#include "modbus/modbusmaster.h"
#include "modbus/modbusslave.h"
#include "templates/algorithms.hpp"
#include "websocket/websocketclient.h"
#include "raw_sockets/udpproducer.h"
#include "raw_sockets/udpconsumer.h"
#include "settings-parsing/adapters/toml.hpp"
#include "settings-parsing/adapters/yaml.hpp"
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
    parseCommandlineArgs();
    m_parser.setApplicationDescription("Redis Adapter");
    preInit();
    initRedis();
    initModbus();
    initWebsockets();
    initSql();
    initSockets();
    initMocks();
    initLocalization();
}


void Launcher::addWorker(Worker* worker, QSet<InterceptorBase*> interceptors)
{
    m_workers.insert(worker, interceptors);
}

//! Все эти классы сохраняютс япри инициализации, логгинг их количества нужен,
//! чтобы компилятор не удалил "неиспользуемые переменные"
void Launcher::preInit()
{
    initLogging();
    qSetMessagePattern(RADAPTER_CUSTOM_MESSAGE_PATTERN);
    try {
        setSettingsPath("bindings");
        auto jsonBindings = JsonBinding::parseMap(readSettings().toMap());
        BindingsProvider::init(jsonBindings);
        reDebug() << "config: Json Bindings count: " << jsonBindings.size();
    } catch (std::runtime_error &e) {}
    try {
        setSettingsPath("redis");
        auto redisServers = parseArray<Settings::RedisServer>("redis.server");
        reDebug() << "config: RedisServer count: " << redisServers.size();
    } catch (std::runtime_error &e) {}
    try {
        setSettingsPath("sql");
        auto sqlClientsInfo = parseArray<Settings::SqlClientInfo>("mysql.client");
        reDebug() << "config: Sql clients count: " << sqlClientsInfo.size();
    } catch (std::runtime_error &e) {}
    try {
        setSettingsPath("modbus");
        auto mbDevices = parseArray<Settings::ModbusDevice>("modbus.devices");
        reDebug() << "config: Modbus devices count: " << mbDevices.size();
    } catch (std::runtime_error &e) {}
    try {
        setSettingsPath("registers");
        Settings::parseRegisters(readSettings());
    } catch (std::runtime_error &e) {}
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

void Launcher::preInitFilters()
{
    try {
        setSettingsPath("filters");
    }   catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "filters" << ")" << " No Filters Found!";
        return;
    }
    auto rawFilters = readSettings("filters").toMap();
    for (auto iter = rawFilters.constBegin(); iter != rawFilters.constEnd(); ++iter) {
        Settings::Filters::table().insert(iter.key(), convertQMap<double>(iter.value().toMap()));
    }
}

void Launcher::initLogging()
{
    try {
        setSettingsPath("config");
        auto rawMap = readSettings("log_debug").toMap();
        auto flattened = JsonDict{rawMap}.flatten(".");
        setLoggingFilters(convertQMap<bool>(flattened));
    } catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "config" << ")" << " No logging settings found!";
    }
}

void Launcher::initGui()
{
#ifdef RADAPTER_GUI
    try {
        setSettingsPath("gui");
        auto guiSettings = parseObj<GuiSettings>("gui");
        if (!guiSettings.enabled) {
            reWarn() << "Gui disabled!";
            return;
        }
        auto guiThread = new QThread(this);
        auto ui = new Gui::MainWindow();
        ui->moveToThread(guiThread);
        connect(this, &Launcher::started, this, [guiThread](){guiThread->start();});
    }
    catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "gui" << ")" << " No Gui settings found!";
    }
#endif
}

void Launcher::initLocalization()
{
    try {
        setSettingsPath("config");
        auto localizationInfo = parseObj<Settings::LocalizationInfo>("localization");
        Localization::instance()->applyInfo(localizationInfo);
        LocalStorage::init(this);
    } catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "config" << ")" << " No localization settings found!";
    }
}

void Launcher::initMocks()
{
    try {
        setSettingsPath("mocks");
        for (const auto &mockSettings : parseArray<Radapter::MockWorkerSettings>("mock")) {
            addWorker(new Radapter::MockWorker(mockSettings, new QThread(this)));
        }
    } catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "mocks" << ")" << " Could not load config for Mocks. Disabling...";
    }
}

void Launcher::initPipelines()
{
    try {
        setSettingsPath("config");
    } catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "config" << ")" << " No Pipeline settings found!";
        return;
    }
    const auto parsed = parseObj<Settings::Pipelines>();
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
    try {setSettingsPath("sockets");}
    catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "sockets" << ")" << " Could not load config for Sockets. Disabling...";
        return;
    }
    for (const auto &udp : parseArray<Udp::ProducerSettings>("socket.udp.producer")) {
        addWorker(new Udp::Producer(udp, new QThread(this)));
    }
    for (const auto &udp : parseArray<Udp::ConsumerSettings>("socket.udp.consumer")) {
        addWorker(new Udp::Consumer(udp, new QThread(this)));
    }
}

void Launcher::initRedis()
{
    try {setSettingsPath("redis");} catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "redis" << ")" << " Could not load config for Redis. Disabling...";
        return;
    }
    for (auto &streamConsumer : parseArray<Settings::RedisStreamConsumer>("redis.stream.consumer")) {
        addWorker(new Redis::StreamConsumer(streamConsumer, new QThread(this)));
    }
    for (auto &streamProducer : parseArray<Settings::RedisStreamProducer>("redis.stream.producer")) {
        addWorker(new Redis::StreamProducer(streamProducer, new QThread(this)));
    }
    for (auto &cacheConsumer : parseArray<Settings::RedisCacheConsumer>("redis.cache.consumer")) {
        addWorker(new Redis::CacheConsumer(cacheConsumer, new QThread(this)));
    }
    for (auto &cacheProducer : parseArray<Settings::RedisCacheProducer>("redis.cache.producer")) {
        addWorker(new Redis::CacheProducer(cacheProducer, new QThread(this)));
    }
    for (auto &keyEventConsumer : parseArray<Settings::RedisKeyEventSubscriber>("redis.keyevents.subscriber")) {
        addWorker(new Redis::KeyEventsConsumer(keyEventConsumer, new QThread(this)));
    }
    //auto redisStreamGroupConsumers = readFromToml<Settings::RedisStreamGroupConsumer>("redis.stream.group_consumer");
}

void Launcher::initModbus()
{
    try {
        setSettingsPath("modbus");
    } catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "modbus" << ")" << " Could not load config for Modbus. Disabling...";
        return;
    }
    for (const auto& slaveInfo : parseArray<Settings::ModbusSlave>("modbus.slave")) {
        addWorker(new Modbus::Slave(slaveInfo, new QThread(this)));
    }
    for (const auto& masterInfo : parseArray<Settings::ModbusMaster>("modbus.master")) {
        addWorker(new Modbus::Master(masterInfo, new QThread(this)));
    }
}

void Launcher::initWebsockets()
{
    try {
        setSettingsPath("websockets");
    } catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "websockets" << ")" << " Could not load config for websockets. Disabling...";
        return;
    }
    for (auto &wsClient : parseArray<Settings::WebsocketClientInfo>("websocket.client")) {
        addWorker(new Websocket::Client(wsClient, new QThread(this)));
    }
    auto websocketServer = parseObj<Settings::WebsocketServerInfo>("websocket.server");
    if (websocketServer.isValid()) {
        addWorker(new Websocket::ServerConnector(websocketServer, new QThread(this)));
    }
}

void Launcher::initSql()
{
    try {
        setSettingsPath("sql");
    } catch (std::runtime_error &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "sql" << ")" << " Could not load config for sql. Disabling...";
        return;
    }
    for (auto &archive : parseArray<Settings::SqlStorageInfo>("mysql.storage.archive")) {
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
    m_parser.addHelpOption();
    m_parser.addVersionOption();
    m_parser.addOptions({
                      {{"d", "directory"},
                        "Config will be read from <directory>", "directory", "conf"},
                      {{"f", "format"},
                        "Config will be read from yaml/toml files", "format", "yaml"}
                      });
    m_parser.process(*QCoreApplication::instance());
    m_configsDir = m_parser.value("directory");
    m_configsFormat = m_parser.value("format");
    if (m_configsFormat == "toml") {
        m_reader = new Settings::TomlReader("conf/config", this);
    } else if (m_configsFormat == "yaml") {
        m_reader = new Settings::YamlReader("conf/config", this);
    } else {
        throw std::runtime_error("Invalid configs format: " + m_configsFormat.toStdString() + "; Can be (toml/yaml)");
    }
}

Broker *Launcher::broker() const
{
    return Broker::instance();
}

QVariant Launcher::readSettings(const QString &tomlPath) {
    return m_reader->get(tomlPath);
}

void Launcher::setSettingsPath(const QString &tomlPath) {
    m_reader->setPath(m_configsDir + "/" + tomlPath);
}

void Launcher::init()
{
    for (auto workerIter = m_workers.begin(); workerIter != m_workers.end(); ++workerIter) {
        Broker::instance()->registerProxy(workerIter.key()->createProxy(workerIter.value()));
    }
#ifdef Q_OS_UNIX
    auto resmon = new ResourceMonitor(this);
    resmon->run();
#endif
}

void Launcher::run()
{
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
    return m_parser;
}

const QString &Launcher::configsDirectory() const
{
    return m_configsDir;
}

