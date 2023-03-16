#include "dictreader.h"

using namespace Settings;

DictReader::DictReader(const QString &resource, const QString &path, QObject *parent) :
    Reader(resource, path, parent)
{
}

QVariant DictReader::get(const QString &key) {
    auto copy = key;
    if (m_config.isEmpty()) {
        m_config = getAll().toMap();
    }
    if (key.isEmpty()) {
        return m_config;
    }
    auto fullKey = copy.replace(".", ":").split(":");
    QVariant result = m_config;
    for (const auto &subkey: fullKey) {
        auto asMap = result.toMap();
        if (result.type() != QVariant::Map || !asMap.contains(subkey)) {
            throw std::runtime_error("[Reader]: Key '" + key.toStdString() + "' was not found! Error in: " +
                                     (resource() + "/" + path()).toStdString());
        }
        result = asMap[subkey];
    }
    return result;
}

void DictReader::setResource(const QString path)
{
    m_config.clear();
    Reader::setResource(path);
}

void DictReader::setPath(const QString &path)
{
    m_config.clear();
    Reader::setPath(path);
}
