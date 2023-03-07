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
#include "producers/producerfilter.h"
#include "producers/sqlarchiveproducer.h"
#include "producers/redisstreamproducer.h"
#include "producers/rediscacheproducer.h"
#include "settings/modbussettings.h"
#include "settings/redissettings.h"
#include "modbus/modbusmaster.h"
#include "modbus/modbusslave.h"
#include "websocket/websocketclient.h"
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
    m_filereader(new Settings::FileReader("conf/config.toml", this))
{
    prvInit();
    m_parser.setApplicationDescription("Redis Adapter");
    m_filereader->initParsingMap();
}


void Launcher::addWorker(Worker* worker, QSet<InterceptorBase*> interceptors)
{
    m_workers.insert(worker, interceptors);
}

//! Все эти классы сохраняютс япри инициализации, логгинг их количества нужен,
//! чтобы компилятор не удалил "неиспользуемые переменные"
void Launcher::preInit()
{
    parseCommandlineArgs();
    initLogging();
    qSetMessagePattern(RADAPTER_CUSTOM_MESSAGE_PATTERN);
    try {
        setTomlPath("bindings.toml");
        auto jsonBindings = JsonBinding::parseMap(m_filereader->deserialise().toMap());
        BindingsProvider::init(jsonBindings);
        reDebug() << "config: Json Bindings count: " << jsonBindings.size();
    } catch (std::exception &e) {}
    try {
        setTomlPath("redis.toml");
        auto redisServers = parseTomlArray<Settings::RedisServer>("redis.server");
        reDebug() << "config: RedisServer count: " << redisServers.size();
    } catch (std::exception &e) {}
    try {
        setTomlPath("sql.toml");
        auto sqlClientsInfo = parseTomlArray<Settings::SqlClientInfo>("mysql.client");
        reDebug() << "config: Sql clients count: " << sqlClientsInfo.size();
    } catch (std::exception &e) {}
    try {
        setTomlPath("modbus.toml");
        auto mbDevices = parseTomlArray<Settings::ModbusDevice>("modbus.devices");
        reDebug() << "config: Modbus devices count: " << mbDevices.size();
    } catch (std::exception &e) {}
    try {
        setTomlPath("registers.toml");
        Settings::parseRegisters(readToml());
    } catch (std::exception &e) {}
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
        setTomlPath("filters.toml");
    }   catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "filters.toml" << ")" << " No Filters Found!";
        return;
    }
    auto rawFilters = readToml("filters").toMap();
    for (auto iter = rawFilters.constBegin(); iter != rawFilters.constEnd(); ++iter) {
        Settings::Filters::table().insert(iter.key(), Serializer::convertQMap<double>(iter.value().toMap()));
    }
}

void Launcher::initLogging()
{
    try {
        setTomlPath("config.toml");
        auto rawMap = m_filereader->deserialise("log_debug").toMap();
        auto flattened = JsonDict{rawMap}.flatten(".");
        setLoggingFilters(Serializer::convertQMap<bool>(flattened));
    } catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "config.toml" << ")" << " No logging settings found!";
    }
}

void Launcher::initGui()
{
#ifdef RADAPTER_GUI
    try {
        setTomlPath("gui.toml");
        auto guiSettings = parseTomlObj<GuiSettings>("gui");
        if (!guiSettings.enabled) {
            reWarn() << "Gui disabled!";
            return;
        }
        auto guiThread = new QThread(this);
        auto ui = new Gui::MainWindow();
        ui->moveToThread(guiThread);
        connect(this, &Launcher::started, guiThread, &QThread::start);
    }
    catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "gui.toml" << ")" << " No Gui settings found!";
    }
#endif
}

void Launcher::prvInit()
{
    preInit();
    initRedis();
    initModbus();
    initWebsockets();
    initSql();
    try {
        setTomlPath("mocks.toml");
        for (const auto &mockSettings : parseTomlArray<Radapter::MockWorkerSettings>("mock")) {
            addWorker(new Radapter::MockWorker(mockSettings, new QThread(this)));
        }
    } catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "mocks.toml" << ")" << " Could not load config for Mocks. Disabling...";
    }
    try {
        setTomlPath("config.toml");
        auto localizationInfo = parseTomlObj<Settings::LocalizationInfo>("localization");
        Localization::instance()->applyInfo(localizationInfo);
        LocalStorage::init(this);
    } catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "config.toml" << ")" << " No localization settings found!";
    }
}

void Launcher::initRedis()
{
    try {setTomlPath("redis.toml");} catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "redis.toml" << ")" << " Could not load config for Redis. Disabling...";
        return;
    }
    for (auto &streamConsumer : parseTomlArray<Settings::RedisStreamConsumer>("redis.stream.consumer")) {
        addWorker(new Redis::StreamConsumer(streamConsumer, new QThread(this)));
    }
    for (auto &streamProducer : parseTomlArray<Settings::RedisStreamProducer>("redis.stream.producer")) {
        addWorker(new Redis::StreamProducer(streamProducer, new QThread(this)));
    }
    for (auto &cacheConsumer : parseTomlArray<Settings::RedisCacheConsumer>("redis.cache.consumer")) {
        addWorker(new Redis::CacheConsumer(cacheConsumer, new QThread(this)));
    }
    for (auto &cacheProducer : parseTomlArray<Settings::RedisCacheProducer>("redis.cache.producer")) {
        addWorker(new Redis::CacheProducer(cacheProducer, new QThread(this)));
    }
    for (auto &keyEventConsumer : parseTomlArray<Settings::RedisKeyEventSubscriber>("redis.keyevents.subscriber")) {
        addWorker(new Redis::KeyEventsConsumer(keyEventConsumer, new QThread(this)));
    }
    //auto redisStreamGroupConsumers = readFromToml<Settings::RedisStreamGroupConsumer>("redis.stream.group_consumer");
}

void Launcher::initModbus()
{
    try {
        setTomlPath("modbus.toml");
    } catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "modbus.toml" << ")" << " Could not load config for Modbus. Disabling...";
        return;
    }
    for (const auto& slaveInfo : parseTomlArray<Settings::ModbusSlave>("modbus.slave")) {
        addWorker(new Modbus::Slave(slaveInfo, new QThread(this)));
    }
    for (const auto& masterInfo : parseTomlArray<Settings::ModbusMaster>("modbus.master")) {
        addWorker(new Modbus::Master(masterInfo, new QThread(this)));
    }
}

void Launcher::initWebsockets()
{
    try {
        setTomlPath("websockets.toml");
    } catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "websockets.toml" << ")" << " Could not load config for websockets. Disabling...";
        return;

    }
    for (auto &wsClient : parseTomlArray<Settings::WebsocketClientInfo>("websocket.client")) {
        addWorker(new Websocket::Client(wsClient, new QThread(this)));
    }
    auto websocketServer = parseTomlObj<Settings::WebsocketServerInfo>("websocket.server");
    if (websocketServer.isValid()) {
        addWorker(new Websocket::ServerConnector(websocketServer, new QThread(this)));
    }

}

void Launcher::initSql()
{
    try {
        setTomlPath("sql.toml");
    } catch (std::exception &e) {
        reWarn().noquote().nospace() << "(" << m_configsDir << "/" << "sql.toml" << ")" << " Could not load config for sql. Disabling...";
        return;
    }
    for (auto &archive : parseTomlArray<Settings::SqlStorageInfo>("mysql.storage.archive")) {
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
                        "Config will be read from <directory>", "directory", "conf"}
                      });
    m_parser.process(*QCoreApplication::instance());
    m_configsDir = m_parser.value("directory");
}

QVariant Launcher::readToml(const QString &tomlPath) {
    return m_filereader->deserialise(tomlPath);
}

void Launcher::setTomlPath(const QString &tomlPath) {
    m_filereader->setPath(m_configsDir + "/" + tomlPath);
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

