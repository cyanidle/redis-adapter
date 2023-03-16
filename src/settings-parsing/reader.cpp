#include "reader.h"

using namespace Settings;

Settings::Reader::Reader(const QString &resource, const QString &path, QObject *parent) :
    QObject(parent),
    m_resource(resource),
    m_path(path)
{
}

QVariant Settings::Reader::operator[](const QString &key) {
    return get(key);
}

const QString &Settings::Reader::resource() const
{
    return m_resource;
}

void Settings::Reader::setResource(const QString path)
{
    m_resource = path;
}

const QString &Settings::Reader::path() const {
    return m_path;
}

void Settings::Reader::setPath(const QString &path)
{
    m_path = path;
}
