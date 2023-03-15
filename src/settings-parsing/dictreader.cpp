#include "dictreader.h"

Settings::Reader::Reader(const QString &path, QObject *parent) :
    QObject(parent),
    m_path(path)
{

}

QVariant Settings::Reader::operator[](QString key) {
    return get(key);
}

const QString &Settings::Reader::path() const {
    return m_path;
}

void Settings::Reader::setPath(const QString &path)
{
    m_path = path;
    onPathSet();
}

Settings::DictReader::DictReader(const QString &path, QObject *parent) :
    Reader(path, parent)
{
}

QVariant Settings::DictReader::get(QString key) {
    if (m_config.isEmpty()) {
        onPathSet();
        m_config = parse();
    }
    if (key.isEmpty()) {
        return m_config;
    }
    auto fullKey = key.replace(".", ":").split(":");
    QVariant result = m_config;
    for (const auto &subkey: fullKey) {
        auto asMap = result.toMap();
        if (result.type() != QVariant::Map || !asMap.contains(subkey)) {
            throw std::runtime_error("[Reader]: Key " + key.toStdString() + " was not found! Error on: " + subkey.toStdString());
        }
        result = asMap[subkey];
    }
    return result;
}
