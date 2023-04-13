#ifndef RADAPTER_API_H
#define RADAPTER_API_H

#include "broker/workers/worker.h"
#include "private/global.h"

namespace Settings {
struct RadapterApi;
}

namespace Radapter {
class Launcher;
class ApiServer : public Worker {
    Q_OBJECT
    struct Private;
public:
    ApiServer(const Settings::RadapterApi& settings, QThread *thread, Launcher* launcher);
    ~ApiServer();
    void onRun() override;
    void initHandlers();
public slots:
    void onMsg(const Radapter::WorkerMsg& msg) override;
private:
    Private *d;
};


}

#endif //RADAPTER_API_H
