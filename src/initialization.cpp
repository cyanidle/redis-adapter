#include "initialization.h"
#include "broker/broker.h"
#include "broker/interceptor/interceptor.h"
#include "broker/workers/fileworker.h"
#include "broker/workers/mockworker.h"
#include "broker/workers/pythonmoduleworker.h"
#include "broker/workers/repeaterworker.h"
#include "broker/workers/settings/fileworkersettings.h"
#include "broker/workers/settings/mockworkersettings.h"
#include "broker/workers/settings/pythonmoduleworkersettings.h"
#include "broker/workers/settings/repeatersettings.h"
#include "broker/workers/worker.h"
#include "interceptors/metainfopipe.h"
#include "interceptors/namespaceunwrapper.h"
#include "interceptors/namespacewrapper.h"
#include "interceptors/remappingpipe.h"
#include "interceptors/renamingpipe.h"
#include "interceptors/settings/namespaceunwrappersettings.h"
#include "interceptors/settings/namespacewrappersettings.h"
#include "interceptors/settings/remappingpipesettings.h"
#include "interceptors/settings/renamingpipesettings.h"
#include "interceptors/settings/validatinginterceptorsettings.h"
#include "interceptors/validatinginterceptor.h"
#include <QStringBuilder>
#include "launcher.h"
#include "qthread.h"
#include <QRecursiveMutex>
#include "raw_sockets/udpconsumer.h"
#include "raw_sockets/udpproducer.h"
#include "validators/validator_fetch.h"
#include "templates/algorithms.hpp"
#include <QCommandLineParser>
#include "private/pipeoperation.h"
#include "private/parsing_private.h"
#include "websocket/websocketclient.h"
#include "websocket/websocketserver.h"

Q_GLOBAL_STATIC(QRecursiveMutex, staticMutex)

using namespace Radapter;
using namespace Radapter::Private;
using PipeOp = Private::PipeOperation;


bool isFunc(const QString &what)
{
    auto split = what.split('(');
    if (split.size() < 2) return false;
    return split.last().endsWith(')');
}

template<typename T>
T tryExtract(const QString &full, const QStringList &data, int index, const QString &name) {

    if (data.size() <= index) {
        goto err;
    } else {
        auto var = QVariant(data[index]);
        auto status = var.convert(QMetaType::fromType<T>());
        if (!status) goto err;
        return var.value<T>();
    }
err:
    QString msg = "Could not get argument ("%name%") in "%full%" with index["%QString::number(index)%"] with type["%QMetaType::fromType<T>().name()%"]";
    throw std::runtime_error(msg.toStdString());
}

void tryCreateInterceptor(const QString &name, QObject *parent)
{
    Q_UNUSED(parent)
    auto broker = Radapter::Broker::instance();
    auto [func, data] = parseFunc(name);
    if (func == "wrap") {
        auto config = Settings::NamespaceWrapper();
        config.wrap_into = tryExtract<QString>(name, data, 0, "wrap_into");
        broker->registerInterceptor(name, new NamespaceWrapper(config));
    } else if (func == "unwrap") {
        auto config = Settings::NamespaceUnwrapper();
        config.unwrap_from = tryExtract<QString>(name, data, 0, "unwrap_from");
        broker->registerInterceptor(name, new NamespaceUnwrapper(config));
    } else if (func == "block" || func == "allow") {
        auto config = Settings::ValidatingInterceptor();
        if (func == "allow") config.inverse = true;
        auto validator = QStringLiteral("invalidate");
        config.by_validator[validator] = data;
        broker->registerInterceptor(name, new ValidatingInterceptor(config));
    }else if (func == "rename") {
        auto config = Settings::RenamingPipe();
        auto from = tryExtract<QString>(name, data, 0, "rename_from");
        config.renames[from] = tryExtract<QString>(name, data, 1, "rename_to");
        broker->registerInterceptor(name, new RenamingPipe(config));
    }  else if (func == "remap") {
        auto config = Settings::RemappingPipe();
        auto field = tryExtract<QString>(name, data, 0, "field");
        auto remap = Settings::FieldRemap();
        remap.from = tryExtract<QString>(name, data, 1, "remap_from");
        remap.to = tryExtract<QString>(name, data, 2, "remap_to");
        config.remaps[field] = remap;
        broker->registerInterceptor(name, new RemappingPipe(config));
    } else if (func == "add_timestamp") {
        auto config = Settings::ValidatingInterceptor();
        config.by_field.value = {{tryExtract<QString>(name, data, 0, "field"), "set_unix_timestamp"}};
        broker->registerInterceptor(name, new ValidatingInterceptor(config));
    } else if (func == "validate") {
        auto config = Settings::ValidatingInterceptor();
        auto validator = tryExtract<QString>(name, data, 0, "validator");
        QStringList fields;
        if (data.size() < 2) {
            throw std::runtime_error("validate(validator, fields...) needs at least one field!");
        }
        for (int i = 1; i < data.size(); ++i) {
            fields.append(tryExtract<QString>(name, data, i, "field_to_validate"));
        }
        config.by_validator[validator] = fields;
        broker->registerInterceptor(name, new ValidatingInterceptor(config));
    } else if (func == "add_metadata") {
        broker->registerInterceptor(name, new MetaInfoPipe());
    } else {
        throw std::runtime_error("(" + name.toStdString() + ") is not supported in pipe!");
    }
}

void tryCreateWorker(const QString &rawFunction, QObject *parent)
{
    auto broker = Radapter::Broker::instance();
    auto [func, data] = parseFunc(rawFunction);
    if (func == "repeater") {
        auto config = Settings::Repeater();
        config.name = rawFunction;
        broker->registerWorker(new Repeater(config, new QThread(parent)));
    } else if (func == "mock") {
        auto config = Settings::MockWorker();
        config.name = rawFunction;
        broker->registerWorker(new MockWorker(config, new QThread(parent)));
    } else if (func == "file") {
        auto config = Settings::FileWorker();
        config.worker->name = rawFunction;
        config.filepath = tryExtract<QString>(rawFunction, data, 0, "filepath");
        broker->registerWorker(new FileWorker(config, new QThread(parent)));
    } else if (func == "py") {
        auto launcher = qobject_cast<Launcher*>(parent);
        if (!launcher) {
            throw std::runtime_error("Cannot create .py workers without launcher as parent!");
        }
        auto config = Settings::PythonModuleWorker();
        config.worker->name = rawFunction;
        auto prefix = launcher->commandLineParser().value("modules-path");
        config.module_path = prefix%'/'%tryExtract<QString>(rawFunction, data, 0, "filepath")%".py";
        auto settingsPath = tryExtract<QString>(rawFunction, data, 1, "settings_path");
        auto raw = launcher->reader()->get(settingsPath);
        if (!raw.canConvert<QVariantMap>()) {
            throw std::runtime_error("Could not fetch config with key: "+settingsPath.toStdString()+" for worker: "+rawFunction.toStdString());
        }
        config.module_settings = raw.toMap();
        broker->registerWorker(new PythonModuleWorker(config, new QThread(parent)));
    } else if (func == "udp.in") {
        auto config = Udp::ConsumerSettings();
        config.worker->name = rawFunction;
        config.port = tryExtract<quint16>(rawFunction, data, 0, "port");
        broker->registerWorker(new Udp::Consumer(config, new QThread(parent)));
    } else if (func == "udp.out") {
        auto config = Udp::ProducerSettings();
        config.worker->name = rawFunction;
        config.server->host = tryExtract<QString>(rawFunction, data, 0, "host");
        config.server->port = tryExtract<quint16>(rawFunction, data, 1, "port");
        broker->registerWorker(new Udp::Producer(config, new QThread(parent)));
    } else if (func == "websocket.server") {
        auto config = Settings::WebsocketServer();
        config.worker->name = rawFunction;
        config.port = tryExtract<quint16>(rawFunction, data, 0, "port");
        broker->registerWorker(new Websocket::Server(config, new QThread(parent)));
    } else if (func == "websocket.client") {
        auto config = Settings::WebsocketClient();
        config.worker->name = rawFunction;
        config.host = tryExtract<QString>(rawFunction, data, 0, "host");
        config.port = tryExtract<quint16>(rawFunction, data, 1, "port");
        broker->registerWorker(new Websocket::Client(config, new QThread(parent)));
    } else {
        throw std::runtime_error("(" + rawFunction.toStdString() + ") is not supported in pipe!");
    }
}

void tryConnecting(const QString &producer, const QString &consumer, const QStringList &pipe, QObject *parent)
{
    auto broker = Radapter::Broker::instance();
    for (auto &step: pipe) {
        auto existing = broker->getInterceptor(step);
        if (!existing) {
            try {
                tryCreateInterceptor(step, parent);
            } catch (std::exception &exc) {
                throw std::runtime_error(
                    std::string("Interceptor: (") + step.toStdString() + ") not found! "
                                                                         "Could not also create --> " + std::string(exc.what()));
            }
        }
    }
    auto existingProd = broker->getWorker(producer);
    if (!existingProd) {
        try {
            tryCreateWorker(producer, parent);
        } catch (std::exception &exc) {
            throw std::runtime_error(
                std::string("Worker: (") + producer.toStdString() + ") not found! "
                                                                    "Could not also create --> " + std::string(exc.what()));
        }
    }
    auto existingCons = broker->getWorker(consumer);
    if (!existingCons) {
        try {
            tryCreateWorker(consumer, parent);
        } catch (std::exception &exc) {
            throw std::runtime_error(
                std::string("Worker: (") + consumer.toStdString() + ") not found! "
                                                                    "Could not also create --> " + std::string(exc.what()));
        }
    }
    broker->connectTwo(producer, consumer, pipe);
}

void tryCreateBidirConnect(const QString &left, const QString &right, QStringList subpipe, QObject *parent)
{
    auto init = [&](const QString &left, const QString &right, PipeOp::Direction dir){
        QStringList fakePipe;
        for (auto &item: subpipe) {
            if (item.isEmpty()) continue;
            auto [func, data] = parseFunc(item);
            if (func == "branch") {
                switch(dir) {
                case PipeOp::Normal: fakePipe.append("unwrap("%data.join(',')%')');break;
                case PipeOp::Inverted: fakePipe.append("wrap("%data.join(',')%')');break;
                default: throw std::runtime_error("Unreachable");
                }
            } else if (func == "block") {
                switch(dir) {
                case PipeOp::Normal: fakePipe.append("block("%data.join(',')%')');break;
                case PipeOp::Inverted: fakePipe.append("block("%data.join(',')%')');break;
                default: throw std::runtime_error("Unreachable");
                }
            } else if (func == "allow") {
                switch(dir) {
                case PipeOp::Normal: fakePipe.append("allow("%data.join(',')%')');break;
                case PipeOp::Inverted: fakePipe.append("allow("%data.join(',')%')');break;
                default: throw std::runtime_error("Unreachable");
                }
            } else if (func == "rename") {
                switch(dir) {
                case PipeOp::Normal: fakePipe.append("rename("%data.join(',')%')');break;
                case PipeOp::Inverted: fakePipe.append("rename("%reversed(data).join(',')%')');break;
                default: throw std::runtime_error("Unreachable");
                }
            } else if (func == "remap") {
                switch(dir) {
                case PipeOp::Normal: fakePipe.append("remap("%data.join(',')%')');break;
                case PipeOp::Inverted: fakePipe.append("remap("%reversed(data).join(',')%')');break;
                default: throw std::runtime_error("Unreachable");
                }
            } else {
                throw std::runtime_error("Unsupported func in bidirectional branch: " + item.toStdString());
            }
        }
        if (dir == PipeOp::Inverted) {
            fakePipe = reversed(fakePipe);
        }
        tryConnecting(left, right, fakePipe, parent);
    };
    init(left, right, PipeOp::Normal);
    init(right, left, PipeOp::Inverted);
}

static bool isInterceptor(const QString &worker) {
    return worker.startsWith('*');
};

void initPipeline(const QList<PipeOp> &ops, QObject *parent)
{
    for (auto &op: ops) {
        switch(op.type) {
        case PipeOp::Normal:
            tryConnecting(op.left, op.right, op.subpipe, parent);
            continue;
        case PipeOp::Inverted:
            tryConnecting(op.right, op.left, reversed(op.subpipe), parent);
            continue;
        case PipeOp::Bidirectional:
            tryCreateBidirConnect(op.left, op.right, op.subpipe, parent);
            continue;
        default:
            throw std::runtime_error("Unreachable!");
        }
    }
}

void Radapter::initPipeline(const QString& pipe, QObject *parent)
{
    static QRegularExpression splitter("(?: +< +| +> +| +<(?:=[^<=>]*)*=> +)");
    //      example: a < b > *pipe() > c <=> d <=func(data)=> e
    // will capture:  _^_ _^_       _^_ __^__ _^______________
    // capture groups indicate a bidirectional pipe (empty or =pipe=pipe=) or left/right op (> or < symbols)
    QMutexLocker lock(&(*staticMutex));
    settingsParsingWarn() << "Initializing pipeline:" << pipe;
    try{
        auto globalMatch = splitter.globalMatch(pipe);
        QList<PipeOp> ops;
        auto split = pipe.split(splitter, Qt::SkipEmptyParts);
        if (split.size() < 2) {
            throw std::runtime_error("Pipeline length must be more than 2!\n"
                                     "Do not forget spaces and '=' in separators: ' < ', ' > ', ' <=...=> '");
        }
        auto firstWorker = split.constFirst();
        auto lastWorker = split.constLast();
        if (isInterceptor(firstWorker)) {
            throw std::runtime_error("Pipeline cannot begin with an interceptor: " + firstWorker.toStdString());
        }
        if (isInterceptor(lastWorker)) {
            throw std::runtime_error("Pipeline cannot end with an interceptor: " + lastWorker.toStdString());
        }
        PipeOp currentOp;
        currentOp.left = firstWorker;
        for (int i = 1; i < split.size(); ++i) {
            auto currentSplitter = globalMatch.next().captured();
            currentOp.handleSplitter(currentSplitter);
            auto &current = split[i];
            if (isInterceptor(current)) {
                currentOp.subpipe.append(current.replace('*', ""));
                continue;
            }
            currentOp.right = current;
            ops.append(currentOp);
            currentOp.reset();
            currentOp.left = current;
        }
        initPipeline(ops, parent);
    } catch(std::exception &exc) {
        throw std::runtime_error("While initializing pipe: " + pipe.toStdString() + " -->\n" + exc.what());
    }
}


