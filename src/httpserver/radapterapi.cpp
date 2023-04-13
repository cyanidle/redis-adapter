#include "radapterapi.h"
#include "httpserver/radapterapisettings.h"
#include "launcher.h"
#include "radapterconfig.h"
#include <QHttpServer>

using namespace Radapter;

struct ApiServer::Private {
    Settings::RadapterApi settings;
    QHttpServer *server;
    Launcher *launcher;
};

ApiServer::ApiServer(const Settings::RadapterApi &settings, QThread *thread, Launcher *launcher) :
    Worker({"radapter.api"}, thread),
    d(new Private{settings, new QHttpServer(this), launcher})
{
    d->server->route("/", [](){
        return "Hello from Redis Adapter";
    });
    initHandlers();
}

ApiServer::~ApiServer()
{
    delete d;
}

void ApiServer::onRun()
{
    Worker::onRun();
    d->server->listen(QHostAddress{d->settings.host}, d->settings.port);
}

void ApiServer::initHandlers()
{
    d->server->route("/config", [this](){
        return JsonDict(d->launcher->config().serialize()).toBytes(d->settings.json_format);
    });
    d->server->route("/config/schema", [this](){
        return JsonDict(d->launcher->config().schema()).toBytes(d->settings.json_format);
    });
}

void ApiServer::onMsg(const WorkerMsg &msg)
{

}
