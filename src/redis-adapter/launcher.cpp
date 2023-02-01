#include "radapter-broker/broker.h"
#include "radapter-broker/debugging/mockworker.h"
#include "redis-adapter/connectors/websocketserverconnector.h"
#include "launcher.h"
#include "radapterlogging.h"
#include "localstorage.h"
#include "localization.h"
#include "bindings/bindingsprovider.h"
#include "redis-adapter/consumers/redisstreamconsumer.h"
#include "redis-adapter/consumers/rediscacheconsumer.h"
#include "redis-adapter/consumers/rediskeyeventsconsumer.h"
#include "redis-adapter/producers/producerfilter.h"
#include "redis-adapter/producers/sqlarchiveproducer.h"
#include "redis-adapter/producers/redisstreamproducer.h"
#include "redis-adapter/producers/rediscacheproducer.h"
#include "redis-adapter/settings/modbussettings.h"
#include "redis-adapter/settings/redissettings.h"
#include "redis-adapter/connectors/modbusconnector.h"
#include "radapter-broker/debugging/logginginterceptor.h"
#include "redis-adapter/websocket/websocketclient.h"
#include "redis-adapter/workers/modbusslaveworker.h"
#include "radapter-broker/metatypes.h"
#ifdef Q_OS_UNIX
#include "redis-adapter/utils/resourcemonitor.h"
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


void Launcher::addWorker(WorkerBase* worker, QSet<InterceptorBase*> interceptors)
{
    m_workers.insert(worker, interceptors);
}

//! Все эти классы сохраняютс япри инициализации, логгинг их количества нужен,
//! чтобы компилятор не удалил "неиспользуемые переменные"
void Launcher::preInit()
{
    parseCommandlineArgs();
    qSetMessagePattern(CUSTOM_MESSAGE_PATTERN);
    setLoggingFilters(Settings::LoggingInfoParser::parse(m_filereader->deserialise("log_debug").toMap()));

    setTomlPath("bindings.toml");
    auto jsonBindings = JsonBinding::parseMap(m_filereader->deserialise().toMap());
    BindingsProvider::init(jsonBindings);
    reDebug() << "config: Json Bindings count: " << jsonBindings.size();

    setTomlPath("redis.toml");
    auto redisServers = parseTomlArray<Settings::RedisServer>("redis.server");
    reDebug() << "config: RedisServer count: " << redisServers.size();
    setTomlPath("sql.toml");
    auto sqlClientsInfo = parseTomlArray<Settings::SqlClientInfo>("mysql.client");
    for (auto &client : sqlClientsInfo) {
        client.table().insert(client.name, client);
    }
    reDebug() << "config: Sql clients count: " << sqlClientsInfo.size();

    setTomlPath("modbus.toml");
    auto tcpDevices = parseTomlArray<Settings::TcpDevice>("modbus.tcp.devices");
    reDebug() << "config: Tcp devices count: " << tcpDevices.size();
    auto rtuDevices = parseTomlArray<Settings::SerialDevice>("modbus.rtu.devices");
    reDebug() << "config: Rtu devices count: " << rtuDevices.size();
}


void Launcher::setLoggingFilters(const Settings::LoggingInfo &loggers)
{
    auto filterRules = QStringList{
        "*.debug=false",
        "default.debug=true"
    };
    for (auto logInfo = loggers.begin(); logInfo != loggers.end(); logInfo++) {
        auto rule = QString("%1=%2").arg(logInfo.key(), logInfo.value() ? "true" : "false");
        filterRules.append(rule);
    }
    if (!filterRules.isEmpty()) {
        Broker::instance()->addDebugMode(loggers);
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
    for (const auto& slaveInfo : parseTomlArray<Settings::ModbusSlaveWorker>("modbus_slave")) {
        addWorker(new Modbus::SlaveWorker(slaveInfo, new QThread(this)));
    }
    auto modbusConnSettings = parseTomlObj<Settings::ModbusConnectionSettings>("modbus");
    if (modbusConnSettings.isValid()) {
        setTomlPath("registers.toml");
        auto mbRegisters = Settings::DeviceRegistersInfoMapParser::parse(readToml("registers").toMap());
        ModbusConnector::init(modbusConnSettings, mbRegisters, new QThread(this));
        addWorker(ModbusConnector::instance());
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
        addWorker(new Sql::ArchiveProducer(archive, new QThread(this)));
    }
    for (auto &archive : parseTomlArray<Settings::SqlStorageInfo>("mysql.storage.archive")) {
        addWorker(new Sql::ArchiveProducer(archive, new QThread(this)));
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

