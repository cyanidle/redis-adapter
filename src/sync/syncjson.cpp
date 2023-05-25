#include "syncjson.h"

using namespace Radapter::Sync;

Json::Json(const JsonDict &start) :
    m_current(start)
{}

JsonDict &Json::current()
{
    return m_current;
}

JsonDict &Json::target()
{
    return m_target;
}

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

JsonDict Json::updateCurrent(const JsonDict &newState)
{
    auto was = m_current;
    m_current.merge(newState);
    return m_current - was;
}

static JsonDict convert(const QStringList& key, const QVariant& val)
{
    JsonDict res;
    res[key] = val;
    return res;
}

JsonDict Json::updateCurrent(const QString &key, const QVariant &val, QChar sep)
{
    return updateCurrent(convert(key.split(sep), val));
}

JsonDict Json::updateCurrent(const QStringList &key, const QVariant &val)
{
    return updateCurrent(convert(key, val));
}

JsonDict Json::updateTarget(const JsonDict &newState)
{
    auto was = m_target;
    m_target.update(newState);
    return m_target - was;
}

JsonDict Json::updateTarget(const QString &key, const QVariant &val, QChar sep)
{
    return updateTarget(convert(key.split(sep), val));
}

JsonDict Json::updateTarget(const QStringList &key, const QVariant &val)
{
    return updateTarget(convert(key, val));
}
