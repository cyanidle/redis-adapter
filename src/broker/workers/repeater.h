#ifndef RADAPTER_REPEATER_H
#define RADAPTER_REPEATER_H

#include "worker.h"
#include <QObject>
namespace Settings {
struct Repeater;
}
namespace Radapter {

class Repeater : public Radapter::Worker
{
    Q_OBJECT
public:
    Repeater(const Settings::Repeater &settings, QThread* thread);
public slots:
    void onMsg(const Radapter::WorkerMsg &msg);
};

} // namespace Radapter

#endif // RADAPTER_REPEATER_H
