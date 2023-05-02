#ifndef RADAPTER_JSON_STATE_H
#define RADAPTER_JSON_STATE_H
#include "serializable/serializableobject.h"
#include "serializable/bindable.hpp"
#include "state/private/privjsonstateqobject.h"
class JsonDict;
namespace Radapter {
struct JsonState : protected Serializable::Object, Private::JsonStateQObject {
    Q_GADGET
public:
    using Serializable::Object::field;
    using Serializable::Object::fields;
    using Serializable::Object::schema;
    using Serializable::Object::metaObject;
    JsonDict send(const QString &fieldName = {}) const;
    bool updateWith(const JsonDict &data);
    template <typename...Args>
    void afterUpdate(Args&&...args) {
        connect(this, &JsonState::wasUpdated, std::forward<Args>(args)...);
    }
};
}

using Serializable::Plain;
using Serializable::Sequence;
using Serializable::Mapping;
using Serializable::Bindable;

#endif //RADAPTER_JSON_STATE_H
