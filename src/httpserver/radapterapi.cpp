#include "radapterapi.h"
#include "broker/broker.h"
#include "httpserver/radapterapisettings.h"
#include "consumers/rediscacheconsumer.h"
#include "producers/rediscacheproducer.h"
#include "commands/rediscommands.h"
#include "launcher.h"
#include "radapterconfig.h"
#include <QHttpServer>
#include <QtConcurrent>

using namespace Radapter;

struct ApiServer::Private {
    Settings::RadapterApi settings;
    QHttpServer *server;
    Launcher *launcher;
};
auto asFuture = [](QHttpServerResponse::StatusCode code){
    return QtConcurrent::run([code]{
        return QHttpServerResponse{code};
    });
};
using Code = QHttpServerResponse::StatusCode;
using Method = QHttpServerRequest::Method;
using Methods = QHttpServerRequest::Methods;
using Future = QFuture<QHttpServerResponse>;
using Responce = QHttpServerResponse;
using Promise = QPromise<QHttpServerResponse>;
QHttpServerResponse basicResponce(JsonDict body, Code code) {
    return QHttpServerResponse(body.toJsonObj(), code);
}

ApiServer::ApiServer(const Settings::RadapterApi &settings, QThread *thread, Launcher *launcher) :
    Worker({"internal.radapter.api"}, thread),
    d(new Private{settings, new QHttpServer(this), launcher})
{
    d->server->route("/", [](){
        return "Hello from Redis Adapter";
    });
    configEndpoints();
    redisEndpoints();
}

ApiServer::~ApiServer()
{
    delete d;
}

void ApiServer::onRun()
{
    d->server->listen(QHostAddress{d->settings.host}, d->settings.port);
    Worker::onRun();
}

void ApiServer::serviceEndpoints()
{
    d->server->setMissingHandler([this](const QHttpServerRequest &request,
                                    QHttpServerResponder &&responder){
        workerWarn(this) << "Unknow endpoint:" << request.url() << ". Access from:" << request.remoteAddress();
        responder.writeStatusLine(Code::NotFound);
    });
}

void ApiServer::configEndpoints()
{
    d->server->route("/config", [this](){
        return JsonDict(d->launcher->config().serialize(), ':', false).toBytes(d->settings.json_format);
    });
    d->server->route("/config/schema", [this](){
        return JsonDict(d->launcher->config().schema(), ':', false).toBytes(d->settings.json_format);
    });
}

void ApiServer::redisEndpoints()
{
    d->server->route("/redis/cache/object/<arg>", Method::Get, [this](const QString &objName, const QHttpServerRequest &request) {
        auto workerName = request.query().queryItemValue("worker");
        auto executor = broker()->getWorker(workerName)->as<Redis::CacheConsumer>();
        if (!executor) {
            return asFuture(Code::NotFound);
        }
        auto promise = std::make_shared<Promise>();
        auto future = promise->future();
        promise->start();
        auto command = prepareCommand(new Redis::Cache::ReadObject(objName));
        command.receivers() = {executor};
        command.setCallback(this, [this, promise] (const ReplyJson *reply) mutable {
            promise->addResult(Responce(reply->as<ReplyJson>()->json().toBytes(d->settings.json_format)));
            promise->finish();
        });
        command.setFailCallback(this, [promise] (const ReplyFail *reply) mutable {
            Responce result(Code::ServiceUnavailable);
            result.addHeader("Fail-Reason", (reply->as<ReplyFail>() ? reply->as<ReplyFail>()->reason() : "Not given").toUtf8());
            promise->addResult(std::move(result));
            promise->finish();
        });
        emit sendMsg(command);
        return future;
    });

}
