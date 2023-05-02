#ifndef RADAPTER_PRIVATE_JSONSTATEQOBJECT_H
#define RADAPTER_PRIVATE_JSONSTATEQOBJECT_H

#include <QObject>

namespace Radapter {
struct JsonState;
namespace Private {
class JsonStateQObject : public QObject
{
    Q_OBJECT
signals:
    void wasUpdated(const Radapter::JsonState *state);
};

} // namespace Private
} // namespace Radapter

#endif // RADAPTER_PRIVATE_JSONSTATEQOBJECT_H
