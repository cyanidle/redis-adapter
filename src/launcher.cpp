#include "broker/broker.h"
#include "broker/worker/mockworker.h"
#include "connectors/websocketserverconnector.h"
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
#include "broker/metatypes.h"
#ifdef Q_OS_UNIX
#include "utils/resourcemonitor.h"
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
    Metatypes::registerAll();
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
    qSetMessagePattern(RADAPTER_CUSTOM_MESSAGE_PATTERN);

    setTomlPath("bindings.toml");
    auto jsonBindings = JsonBinding::parseMap(m_filereader->deserialise().toMap());
    BindingsProvider::init(jsonBindings);
    reDebug() << "config: Json Bindings count: " << jsonBindings.size();

    setTomlPath("redis.toml");
    auto redisServers = parseTomlArray<Settings::RedisServer>("redis.server");
    reDebug() << "config: RedisServer count: " << redisServers.size();
    setTomlPath("sql.toml");
    auto sqlClientsInfo = parseTomlArray<Settings::SqlClientInfo>("mysql.client");
    reDebug() << "config: Sql clients count: " << sqlClientsInfo.size();

    setTomlPath("modbus.toml");
    auto mbDevices = parseTomlArray<Settings::ModbusDevice>("modbus.devices");
    reDebug() << "config: Modbus devices count: " << mbDevices.size();

    setTomlPath("registers.toml");
    Settings::parseRegisters(readToml());
}


void Launcher::setLoggingFilters(const QMap<QString, bool> &loggers)
{
    auto filterRules = QStringList{
        "*.debug=false",
        "all-workers=true",
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
    if (!filterRules.isEmpty()) {
        Broker::instance()->applyLoggingFilters(loggers);
        auto filterString = filterRules.join("\n");
        QLoggingCategory::setFilterRules(filterString);
    }
}

void Launcher::preInitFilters()
{
    setTomlPath("filters.toml");
    auto rawFilters = readToml("filters").toMap();
    for (auto iter = rawFilters.constBegin(); iter != rawFilters.constEnd(); ++iter) {
        Settings::Filters::table().insert(iter.key(), Serializer::convertQMap<double>(iter.value().toMap()));
    }
}

void Launcher::initLogging()
{
    setTomlPath("config.toml");
    auto rawMap = m_filereader->deserialise("log_debug").toMap();
    auto flattened = JsonDict{rawMap}.flatten(".");
    setLoggingFilters(Serializer::convertQMap<bool>(flattened));
}


void Launcher::prvInit()
{
    preInit();
    initRedis();
    initModbus();
    initWebsockets();
    initSql();
    setTomlPath("mocks.toml");
    for (const auto &mockSettings : parseTomlArray<Radapter::MockWorkerSettings>("mock")) {
        addWorker(new Radapter::MockWorker(mockSettings, new QThread()));
    }
    setTomlPath("config.toml");
    auto localizationInfo = parseTomlObj<Settings::LocalizationInfo>("localization");
    Localization::instance()->applyInfo(localizationInfo);
    LocalStorage::init(this);
}

void Launcher::initRedis()
{
    setTomlPath("redis.toml");
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
    setTomlPath("modbus.toml");
    for (const auto& slaveInfo : parseTomlArray<Settings::ModbusSlave>("modbus.slave")) {
        addWorker(new Modbus::Slave(slaveInfo, new QThread(this)));
    }
    for (const auto& masterInfo : parseTomlArray<Settings::ModbusMaster>("modbus.master")) {
        addWorker(new Modbus::Master(masterInfo, new QThread(this)));
    }
}

void Launcher::initWebsockets()
{
    setTomlPath("websockets.toml");
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
    setTomlPath("sql.toml");
    for (auto &archive : parseTomlArray<Settings::SqlStorageInfo>("mysql.storage.archive")) {
        addWorker(new MySql::ArchiveProducer(archive, new QThread(this)));
    }
}

void Launcher::parseCommandlineArgs()
{
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

bool Launcher::setTomlPath(const QString &tomlPath) {
    return m_filereader->setPath(m_configsDir + "/" + tomlPath);
}

void Launcher::init()
{
    if (QCoreApplication::applicationName().isEmpty()) {
        QCoreApplication::setApplicationName("redis-adapter");
    }
    if (QCoreApplication::applicationVersion().isEmpty()) {
        QCoreApplication::setApplicationVersion(RADAPTER_VERSION);
    }
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
}

QCommandLineParser &Launcher::commandLineParser()
{
    return m_parser;
}

const QString &Launcher::configsDirectory() const
{
    return m_configsDir;
}

