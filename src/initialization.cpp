#include "initialization.h"
#include "broker/broker.h"
#include "broker/interceptor/interceptor.h"
#include "broker/workers/fileworker.h"
#include "broker/workers/mockworker.h"
#include "broker/workers/repeaterworker.h"
#include "broker/workers/settings/fileworkersettings.h"
#include "broker/workers/settings/mockworkersettings.h"
#include "broker/workers/settings/repeatersettings.h"
#include "broker/workers/worker.h"
#include "interceptors/metainfopipe.h"
#include "interceptors/namespaceunwrapper.h"
#include "interceptors/namespacewrapper.h"
#include "interceptors/settings/namespaceunwrappersettings.h"
#include "interceptors/settings/namespacewrappersettings.h"
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

Q_GLOBAL_STATIC(QRecursiveMutex, staticMutex)

using namespace Radapter;

struct FuncResult {
    QString func;
    QStringList data;
};

bool isFunc(const QString &what)
{
    auto split = what.split('(');
    if (split.size() < 2) return false;
    return split.last().endsWith(')');
}

FuncResult parseFunc(const QString &rawFunc)
{
    static auto splitter = QRegularExpression("\\(.*\\)");
    if (rawFunc.size() < 2) {
        throw std::runtime_error("Attempt to parse invalid pipe function: " + rawFunc.toStdString());
    }
    if (!rawFunc.contains(splitter)) {
        return {rawFunc, {}};
    }
    auto func = rawFunc.split(splitter)[0];
    auto data = splitter
                    .match(rawFunc) // func(data, data)
                    .captured().remove(0, 1).chopped(1) // data, data
                    .split(','); // [data, data]
    for (auto &item: data){
        item = item.simplified();
    };
    return {func, data};
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

void tryCreateWorker(const QString &name, QObject *parent)
{
    auto broker = Radapter::Broker::instance();
    auto [func, data] = parseFunc(name);
    if (func == "repeater") {
        auto config = Settings::Repeater();
        config.name = name;
        broker->registerWorker(new Repeater(config, new QThread(parent)));
    } else if (func == "mock") {
        auto config = Settings::MockWorker();
        config.name = name;
        broker->registerWorker(new MockWorker(config, new QThread(parent)));
    } else if (func == "file") {
        auto config = Settings::FileWorker();
        config.worker->name = name;
        config.filepath = tryExtract<QString>(name, data, 0, "filepath");
        broker->registerWorker(new FileWorker(config, new QThread(parent)));
    } else if (func == "udp.in") {
        auto config = Udp::ConsumerSettings();
        config.worker->name = name;
        config.port = tryExtract<quint16>(name, data, 0, "port");
        broker->registerWorker(new Udp::Consumer(config, new QThread(parent)));
    } else if (func == "udp.out") {
        auto config = Udp::ProducerSettings();
        config.worker->name = name;
        config.server->host = tryExtract<QString>(name, data, 0, "host");
        config.server->port = tryExtract<quint16>(name, data, 1, "port");
        broker->registerWorker(new Udp::Producer(config, new QThread(parent)));
    } else {
        throw std::runtime_error("(" + name.toStdString() + ") is not supported in pipe!");
    }
}

void tryConnecting(const QString &producer, const QString &consumer, const QStringList &pipe, QObject *parent)
{
    auto broker = Radapter::Broker::instance();
    for (auto &step: pipe) {
        auto existing = broker->getInterceptor(step);
        if (!existing) {
            tryCreateInterceptor(step, parent);
        }
    }
    auto existingProd = broker->getWorker(producer);
    if (!existingProd) {
        tryCreateWorker(producer, parent);
    }
    auto existingCons = broker->getWorker(consumer);
    if (!existingCons) {
        tryCreateWorker(consumer, parent);
    }
    broker->connectTwo(producer, consumer, pipe);
}

enum Dir {
    Normal,
    Flipped
};

void tryCreateBidirConnect(QString left, QString right, QString pipe, Dir dir, QObject *parent)
{
    if (dir == Flipped) {
        std::swap(left, right);
    }
    auto realPipe = pipe.replace('<', "")
                        .replace('>', "")
                        .replace('=', "")
                        .simplified()
                        .split(' ');
    for (auto &item: realPipe) {
        item = item.simplified();
    }
    QStringList fakePipe;
    for (auto &item: realPipe) {
        if (item.isEmpty()) continue;
        auto [func, data] = parseFunc(item);
        if (func == "branch") {
            switch(dir) {
            case Normal: fakePipe.append("unwrap("%data.join(',')%')');break;
            case Flipped: fakePipe.append("wrap("%data.join(',')%')');break;
            }
        } else {
            throw std::runtime_error("Unsupported func in bidirectional branch: " + item.toStdString());
        }
    }
    tryConnecting(left, right, fakePipe, parent);
}

static bool isInterceptor (const QString &worker) {
    return worker.startsWith('*');
};

bool handleReverse(QStringList &source, QObject *parent)
{
    auto hadReverse = false;
    for (auto &point: source) {
        if (!point.contains(" < ")) continue;
        hadReverse = true;
        auto realPipe = point.split(" < ");
        std::reverse(realPipe.begin(), realPipe.end());
        initPipe(realPipe.join(" > "), parent);
        point = realPipe.first();
    }
    return hadReverse;
}

bool handleBidirectional(QStringList &source, QObject *parent)
{
    static auto bidirSplitter = QRegularExpression(" <=.*> ");
    auto hadBidirect = false;
    for (auto &point: source) {
        if (!point.contains(bidirSplitter)) continue;
        hadBidirect = true;
        auto pairs = point.split(bidirSplitter);
        auto match = bidirSplitter.match(point);
        if (pairs.size() < 2) {
            throw std::runtime_error("Cannot have bidirectional pair (<=...=>) with only one worker!");
        }
        if (isInterceptor(pairs.first()) || isInterceptor(pairs.last())) {
            throw std::runtime_error("Cannot have bidirectional pipe (<=...=>) starting or ending with interceptors!");
        }
        for (int i = 1; i < pairs.size(); ++i) {
            tryCreateBidirConnect(pairs[i-1], pairs[i], match.captured(i-1), Normal, parent);
            tryCreateBidirConnect(pairs[i-1], pairs[i], match.captured(i-1), Flipped, parent);
            point = pairs[i];
        }
    }
    return hadBidirect;
}

void Radapter::initPipe(const QString& pipe, QObject *parent)
{
    QMutexLocker lock(&(*staticMutex));
    try{
    auto split = pipe.split(" > ");
    auto hadReverse = handleReverse(split, parent);
    auto hadBidirect = handleBidirectional(split, parent);
    if (split.size() < 2) {
        if (!hadBidirect && !hadReverse) {
            throw std::runtime_error("Pipeline length must be more than 2!");
        } else {
            return;
        }
    }
    auto prevWorker = split.takeFirst().simplified();
    auto lastWorker = split.constLast().simplified();
    if (isInterceptor(prevWorker)) {
        throw std::runtime_error("Pipeline cannot begin with an interceptor: " + prevWorker.toStdString());
    }
    if (isInterceptor(lastWorker)) {
        throw std::runtime_error("Pipeline cannot end with an interceptor: " + lastWorker.toStdString());
    }
    QList<QPair<QString, QString>> workers;
    QList<QStringList> interceptors;
    QStringList currentInterceptors;
    for (const auto &point : split) {
        auto current = point.simplified();
        if (isInterceptor(current)) {
            currentInterceptors.append(current.remove(0, 1).simplified());
            continue;
        }
        workers.append({prevWorker, current});
        interceptors.append(currentInterceptors);
        currentInterceptors.clear();
        prevWorker = point.simplified();
    }
    for (auto [pair, interceptorNames]: zip(workers, interceptors)) {
        auto [sourceName, targetName] = pair;
        tryConnecting(sourceName, targetName, interceptorNames, parent);
    }
    } catch(std::exception &exc) {
        throw std::runtime_error("While initializing pipe: " + pipe.toStdString() + " --> \n" + exc.what());
    }
}

void Radapter::initPipelines(const QStringList &pipelines, QObject *parent)
{
    for (const auto &pipe: pipelines) {
        settingsParsingWarn() << "Initializing pipeline:" << pipe;
        initPipe(pipe, parent);
    }
}






