#ifndef RADAPTER_PRIVATE_PIPEOPERATION_H
#define RADAPTER_PRIVATE_PIPEOPERATION_H

#include "private/global.h"

namespace Radapter {
namespace Private {

struct PipeOperation
{
    Q_GADGET
public:
    enum Direction {
        None,
        Normal,
        Inverted,
        Bidirectional
    };
    Q_ENUM(Direction)
    QString left;
    QString right;
    QStringList subpipe;
    Direction type{None};
    void reset();
    void handleSplitter(const QString &splitter);
};

} // namespace Private
} // namespace Radapter
Q_DECLARE_TYPEINFO(Radapter::Private::PipeOperation, Q_MOVABLE_TYPE);
#endif // RADAPTER_PRIVATE_PIPEOPERATION_H
