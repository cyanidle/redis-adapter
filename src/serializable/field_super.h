#ifndef FIELD_SUPER_H
#define FIELD_SUPER_H

#define FIELD_SUPER(cls) \
    using typename cls::valueType; \
    using typename cls::valueRef; \
    using cls::cls; \
    using cls::operator=; \
    using cls::operator==;

#endif // FIELD_SUPER_H
