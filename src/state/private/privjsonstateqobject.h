#ifndef RADAPTER_PRIVATE_JSONSTATEQOBJECT_H
#define RADAPTER_PRIVATE_JSONSTATEQOBJECT_H

#include <QObject>

namespace State {
struct Json;
namespace Private {
class JsonStateQObject : public QObject
{
    Q_OBJECT
signals:
    void wasUpdated(const State::Json *state);
};

} // namespace Private
} // namespace Radapter

#endif // RADAPTER_PRIVATE_JSONSTATEQOBJECT_H
