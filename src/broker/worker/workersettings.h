#ifndef WORKERBASESETTINGS_H
#define WORKERBASESETTINGS_H

#include "settings-parsing/serializer.hpp"

namespace Radapter {

struct RADAPTER_SHARED_SRC WorkerSettings : public Serializer::SerializableGadget  {
    Q_GADGET
    IS_SERIALIZABLE
    SERIAL_FIELD(QString, name);
    SERIAL_CONTAINER(QList, QString, producers, DEFAULT)
    SERIAL_CONTAINER(QList, QString, consumers, DEFAULT)
    SERIAL_CONTAINER(QList, QString, namespaces, DEFAULT)
    WorkerSettings(const QString &name = {}) : name(name) {}
    bool isValid() const {
        return !name.isEmpty();
    }
};

}

#endif // WORKERBASESETTINGS_H
