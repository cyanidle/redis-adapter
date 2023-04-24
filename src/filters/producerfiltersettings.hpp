#ifndef PRODUCERFILTERSETTINGS_HPP
#define PRODUCERFILTERSETTINGS_HPP

#include "settings-parsing/serializablesetting.h"

namespace Settings {

struct ProducerFilter : Serializable {
    Q_GADGET
    IS_SERIALIZABLE
    FIELD(OptionalMapping<double>, by_field)
    FIELD(OptionalMapping<double>, by_wildcard)
};

}


#endif // PRODUCERFILTERSETTINGS_HPP
