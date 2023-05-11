#include "syncjson.h"

using namespace Radapter::Sync;

Json::Json(const JsonDict &start) :
    m_current(start)
{}

const JsonDict &Json::current() const
{
    return m_current;
}

const JsonDict &Json::target() const
{
    return m_target;
}

JsonDict Json::missingToTarget() const
{
    return m_target - m_current;
}

bool Json::updateCurrent(const JsonDict &newState)
{
    return m_current.update(newState);
}

static JsonDict convert(const QStringList& key, const QVariant& val)
{
    JsonDict res;
    res[key] = val;
    return res;
}

bool Json::updateCurrent(const QString &key, const QVariant &val, QChar sep)
{
    return updateCurrent(convert(key.split(sep), val));
}

bool Json::updateCurrent(const QStringList &key, const QVariant &val)
{
    return updateCurrent(convert(key, val));
}

bool Json::updateTarget(const JsonDict &newState)
{
    return m_target.update(newState);
}

bool Json::updateTarget(const QString &key, const QVariant &val, QChar sep)
{
    return updateTarget(convert(key.split(sep), val));
}

bool Json::updateTarget(const QStringList &key, const QVariant &val)
{
    return updateTarget(convert(key, val));
}
