#include "jsonstate.h"
#include "jsondict/jsondict.h"

JsonDict Radapter::JsonState::send(const QString &fieldName) const
{
    if (!fieldName.isEmpty()) {
        auto found = field(fieldName);
        if (!found) {
            reError() << "#### Attempt to send nonexistent field of JsonState:" << fieldName;
            return {};
        }
        return {{fieldName, found->readVariant(this)}};
    }
    return serialize();
}

bool Radapter::JsonState::updateWith(const JsonDict &data)
{
    auto status = update(data);
    if (status) emit wasUpdated(this);
    return status;
}

