#ifndef SETTINGS_EXAMPLE_H
#define SETTINGS_EXAMPLE_H

#include "private/global.h"

namespace Settings {
struct Example;
struct FieldExample
{
    enum NestedType {
        None,
        NestedField,
        NestedSequence,
        NestedMapping,
    };
    enum FieldType {
        FieldPlain,
        FieldSequence,
        FieldMapping,
    };
    QVariant defaultValue;
    QString typeName;
    QStringList attributes;
    QString comment;
    QSharedPointer<Example> nested;
    NestedType nestedType{None};
    FieldType fieldType{FieldPlain};
};

struct Example
{
    QMap<QString, FieldExample> fields;
    QString comment;
};

} // namespace Settings

#endif // SETTINGS_EXAMPLE_H
