#ifndef DUPLICATINGINTERSEPTOR_SETTINGS_H
#define DUPLICATINGINTERSEPTOR_SETTINGS_H

#include "settings-parsing/serializablesettings.h"

namespace Settings {

struct DuplicatingInterceptor : public SerializableSettings {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(RequiredMapping<QStringList>, by_field)

    QMap<QString, QStringList> _by_field;
    POST_UPDATE {
        for (auto iter = by_field->cbegin(); iter != by_field->cend(); ++iter) {
            for (const auto &dup: iter.value()) {
                _by_field[iter.key()].append(dup.split(':'));
            }
        }
    }
};

}

#endif // DUPLICATINGINTERSEPTOR_SETTINGS_H
