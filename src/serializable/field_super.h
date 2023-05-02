#ifndef FIELD_SUPER_H
#define FIELD_SUPER_H

#include "private/global.h"

#define FIELD_SUPER(cls) \
    using typename cls::valueType; \
    using typename cls::valueRef; \
    using cls::cls; \
    using cls::operator=; \
    using cls::operator==; \
    template <typename Class, typename Field> \
    friend struct ::Serializable::Private::FieldHolder; // NOLINT

#endif // FIELD_SUPER_H
