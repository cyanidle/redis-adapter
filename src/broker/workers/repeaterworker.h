#ifndef RADAPTER_REPEATER_H
#define RADAPTER_REPEATER_H

#include "worker.h"

namespace Settings {
struct Repeater;
}
namespace Radapter {

class Repeater : public Radapter::Worker
{
    Q_OBJECT
    struct Private;
public:
    Repeater(const Settings::Repeater &settings, QThread* thread);
    ~Repeater() override;
public slots:
    void onMsg(const Radapter::WorkerMsg &msg) override;
private:
    Private *d;
};

} // namespace Radapter

#endif // RADAPTER_REPEATER_H
