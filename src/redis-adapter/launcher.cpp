#include "radapter-broker/broker.h"
#include "radapter-broker/debugging/mockworker.h"
#include "redis-adapter/factories/rediscachefactory.h"
#include "redis-adapter/factories/redispubsubfactory.h"
#include "redis-adapter/factories/redisstreamfactory.h"
#include "redis-adapter/factories/sqlarchivefactory.h"
#include "redis-adapter/factories/websocketclientfactory.h"
#include "redis-adapter/connectors/websocketserverconnector.h"
#include "launcher.h"
#include "radapterlogging.h"
#include "localstorage.h"
#include "localization.h"
#include "bindings/bindingsprovider.h"
#include "redis-adapter/settings/modbussettings.h"
#include "redis-adapter/settings/redissettings.h"
#include "redis-adapter/connectors/modbusconnector.h"
#include "radapter-broker/debugging/logginginterceptor.h"
#include "redis-adapter/workers/modbusslaveworker.h"
#include "radapter-broker/metatypes.h"
#ifdef Q_OS_UNIX
#include "redis-adapter/utils/resourcemonitor.h"
#endif

using namespace Radapter;

Launcher::Launcher(QObject *parent) :
    QObject(parent),
    m_factories(),
    m_workers(),
    m_sqlFactory(nullptr),
    m_filereader(new Settings::FileReader("conf/config.toml", this))
{
    Metatypes::registerAll();
    m_filereader->initParsingMap();
    prvInit();
}

void Launcher::addFactory(Radapter::FactoryBase* factory)
{
    m_factories.append(factory);
}

void Launcher::addWorker(WorkerBase* worker, QSet<InterceptorBase*> interceptors)
{
    m_workers.insert(worker, interceptors);
}

//! Все эти классы сохраняютс япри инициализации, логгинг их количества нужен,
//! чтобы компилятор не удалил "неиспользуемые переменные"
void Launcher::prvPreInit()
{
    qSetMessagePattern(CUSTOM_MESSAGE_PATTERN);
    setLoggingFilters(Settings::LoggingInfoParser::parse(m_filereader->deserialise("log_debug").toMap()));
    m_filereader->setPath("conf/bindings.toml");
    auto jsonBindings = JsonBinding::parseMap(m_filereader->deserialise().toMap());
    BindingsProvider::init(jsonBindings);
    reDebug() << "config: Json Bindings count: " << jsonBindings.size();
    m_filereader->setPath("conf/config.toml");
    auto redisServers = precacheFromToml<Settings::RedisServer>("redis.servers");
    reDebug() << "config: Redis Servers count: " << redisServers.size();
    auto redisStreams = precacheFromToml<Settings::RedisStream>("redis.streams");
    reDebug() << "config: Redis Streams count: " << redisStreams.size();
    auto redisCaches = precacheFromToml<Settings::RedisCache>("redis.caches");
    reDebug() << "config: Caches count: " << redisCaches.size();
    auto sqlClientsInfo = precacheFromToml<Settings::SqlClientInfo>("mysql.client");
    reDebug() << "config: Sql clients count: " << sqlClientsInfo.size();
    auto redisSubscribers = precacheFromToml<Settings::RedisKeyEventSubscriber>("redis.subscribers");
    reDebug() << "config: Redis Subs count: " << redisSubscribers.size();
    m_filereader->setPath("conf/modbus.toml");
    auto tcpDevices = precacheFromToml<Settings::TcpDevice>("modbus.tcp.devices");
    reDebug() << "config: Tcp devices count: " << tcpDevices.size();
    auto rtuDevices = precacheFromToml<Settings::SerialDevice>("modbus.rtu.devices");
    reDebug() << "config: Rtu devices count: " << rtuDevices.size();
    m_filereader->setPath("conf/config.toml");
    auto keyvaults = precacheFromToml<Settings::SqlKeyVaultInfo>("mysql.keyvault");
    reDebug() << "config: Sql Keyvaults count: " << keyvaults.size();
    m_filereader->setPath("conf/filters.toml");
    auto filters = precacheFromToml<Settings::Filters>("filters");
    reDebug() << "config: Filters count: " << filters.size();
    m_filereader->setPath("conf/config.toml");
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
        Broker::instance()->setDebugMode(loggers);
        auto filterString = filterRules.join("\n");
        QLoggingCategory::setFilterRules(filterString);
    }
}


int Launcher::prvInit()
{
    prvPreInit();
    addFactory(new Redis::StreamFactory(Settings::RedisStream::cacheMap, this));
    addFactory(new Redis::CacheFactory(Settings::RedisCache::cacheMap.values(), this));
    addFactory(new Redis::PubSubFactory(Settings::RedisKeyEventSubscriber::cacheMap.values(), this));
    m_filereader->setPath("conf/mocks.toml");
    const auto mocks = Serializer::fromQList<Radapter::MockWorkerSettings>(
            m_filereader->deserialise("mock").toList()
        );
    for (const auto &mockSettings : mocks) {
        addWorker(new Radapter::MockWorker(mockSettings, new QThread()));
    }
    m_filereader->setPath("conf/modbus.toml");
    auto slavesList = m_filereader->deserialise("modbus_slave").toList();
    if (!slavesList.isEmpty()) {
        auto mbSlaves = Serializer::fromQList<Settings::ModbusSlaveWorker>(slavesList);
        for (const auto& slaveInfo : qAsConst(mbSlaves)) {
            addWorker(new Modbus::SlaveWorker(slaveInfo, new QThread(this)));
        }
    }
    auto modbusConnSettingsRaw = m_filereader->deserialise("modbus", true).toMap();
    if (!modbusConnSettingsRaw.isEmpty()) {
        auto modbusConnSettings = Serializer::fromQMap<Settings::ModbusConnectionSettings>(modbusConnSettingsRaw);
        m_filereader->setPath("conf/registers.toml");
        auto mbRegisters = Settings::DeviceRegistersInfoMapParser::parse(
            m_filereader->deserialise("registers", true).toMap());
        ModbusConnector::init(modbusConnSettings,
                              mbRegisters,
                              modbusConnSettings.worker,
                              new QThread(this));
        QSet<InterceptorBase*> mbInterceptors{};
        if (!modbusConnSettings.filters.isEmpty()) {
            mbInterceptors.insert(new ProducerFilter(modbusConnSettings.filters));
        }
        if (modbusConnSettings.log_jsons) {
            mbInterceptors.insert(new LoggingInterceptor(*modbusConnSettings.log_jsons));
        }
        addWorker(ModbusConnector::instance(), mbInterceptors);
    }
    m_filereader->setPath("conf/config.toml");
    if (!Settings::SqlClientInfo::cacheMap.isEmpty()) {
        m_sqlFactory = new MySqlFactory(Settings::SqlClientInfo::cacheMap.values(), this);
        m_sqlFactory->initWorkers();
        auto archivesInfo = Serializer::fromQList<Settings::SqlStorageInfo>(
            m_filereader->deserialise("mysql.storage.archive", true).toList());
        if (!archivesInfo.isEmpty()) {
            addFactory(new Sql::ArchiveFactory(archivesInfo, m_sqlFactory, this));
        }
    }
    auto websocketClients = Serializer::fromQList<Settings::WebsocketClientInfo>(
        m_filereader->deserialise("websocket.client", true).toList());
    if (!websocketClients.isEmpty()) {
        addFactory(new Websocket::ClientFactory(websocketClients, this));
    }
    auto websocketServerConf = m_filereader->deserialise("websocket.server", true).toMap();
    if (!websocketServerConf.isEmpty()) {
        auto websocketServer = Serializer::fromQMap<Settings::WebsocketServerInfo>(websocketServerConf);
        if (websocketServer.isValid()) {
            addWorker(new Websocket::ServerConnector(websocketServer, websocketServer.worker, new QThread(this)));
        }
    }
    auto localizationInfoMap = m_filereader->deserialise("localization").toMap();
    if (!localizationInfoMap.isEmpty()) {
        auto localizationInfo = Serializer::fromQMap<Settings::LocalizationInfo>(localizationInfoMap);
        Localization::init(localizationInfo, this);
    }
    m_filereader->setPath("conf/config.toml");
    LocalStorage::init(this);
    return 0;
}


int Launcher::init()
{
    int status = 0;
    status = initWorkers();
    if (status != 0) {
        return status;
    }
#ifdef Q_OS_UNIX
    auto resmon = new ResourceMonitor(this);
    resmon->run();
#endif
    return status;
}

int Launcher::initWorkers()
{
    int status = 0;
    for (auto &factory : m_factories) {
        status = factory->initWorkers();
        if (status != 0) {
            return status;
        }
    }
    for (auto workerIter = m_workers.begin(); workerIter != m_workers.end(); ++workerIter) {
        Broker::instance()->registerProxy(workerIter.key()->createProxy(workerIter.value()));
    }
    return 0;
}

void Launcher::run()
{
    Broker::instance()->connectProducersAndConsumers();
    for (auto &factory : m_factories) {
        factory->run();
    }
    for (auto worker = m_workers.keyBegin(); worker != m_workers.keyEnd(); ++worker) {
        (*worker)->run();
    }
    Broker::instance()->runAll();
}

