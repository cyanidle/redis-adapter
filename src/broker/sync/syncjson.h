#ifndef RADAPTER_SYNC_JSON_H
#define RADAPTER_SYNC_JSON_H

#include "jsondict/jsondict.h"

namespace Radapter {
namespace Sync {

class Json
{
public:
    Json(const JsonDict &start = {});
    Json(const Json& other) = default;
    Json(Json&& other) = default;
    Json &operator=(const Json& other) = default;
    Json &operator=(Json&& other) = default;
    [[nodiscard]]
    const JsonDict &current() const;
    [[nodiscard]]
    const JsonDict &target() const;
    JsonDict missingToTarget() const;
    bool updateCurrent(const JsonDict &newState);
    bool updateCurrent(const QString &key, const QVariant &val, QChar sep = ':');
    bool updateCurrent(const QStringList &key, const QVariant &val);
    bool updateTarget(const JsonDict &newState);
    bool updateTarget(const QString &key, const QVariant &val, QChar sep = ':');
    bool updateTarget(const QStringList &key, const QVariant &val);
private:
    JsonDict m_target;
    JsonDict m_current;
};

} // namespace Sync
} // namespace Radapter

#endif // RADAPTER_SYNC_JSON_H
