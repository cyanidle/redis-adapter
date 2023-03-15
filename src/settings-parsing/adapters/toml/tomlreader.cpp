#include "tomlreader.h"
#include "radapterlogging.h"
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimeZone>
#include "cpptoml.h"
#include "../../qmaputils.hpp"
#include "settings-parsing/dictreader.h"

using namespace Settings;

typedef quint16 MbRegister_t;

std::shared_ptr<cpptoml::table> initTable(const QString &path)
{
    auto fullPath = QCoreApplication::applicationDirPath() + "/" + path;
    auto nativePath = QDir::toNativeSeparators(fullPath);
    if (!QFile(QDir(nativePath).absolutePath()).exists()) {
        fullPath = QDir::currentPath()  + "/" + path;
        nativePath = QDir::toNativeSeparators(path);
    }
    return cpptoml::parse_file(nativePath.toStdString());
}

ParsingMap TomlReader::getParsingMap()
{
    try {
        return convertQMap<QString>(get("parsing").toMap());
    } catch (const std::runtime_error &exc) {
        return {};
    }
}

QString decodeString(const QString &encodedString, const ParsingMap &parsingMap)
{
    auto decodedString = encodedString;
    if (parsingMap.isEmpty()) {
        return encodedString;
    }
    for (auto parsingItem = parsingMap.begin();
         parsingItem != parsingMap.end();
         parsingItem++)
    {
        if (decodedString.contains(parsingItem.key())) {
            decodedString.replace(parsingItem.key(), parsingItem.value());
        }
    }
    return decodedString;
}

QVariant deserialise(const std::shared_ptr<cpptoml::base> &base, const ParsingMap &parsingMap)
{
    if (base->is_value()) {
        if (base->as<bool>()){
            return QVariant(base->as<bool>()->get());
        } else if (base->as<std::string>().get()){
            auto currentStr = QString(base->as<std::string>()->get().c_str());
            if (!parsingMap.isEmpty()) {
                return decodeString(currentStr, parsingMap);
            }
            return QVariant(currentStr);
        } else if (base->as<int64_t>()){
            return QVariant(qlonglong(base->as<int64_t>()->get()));
        }  else if (base->as<double>()){
            return QVariant(base->as<double>()->get());
        } else if (base->as<cpptoml::local_time>().get()){
            auto localTime = base->as<cpptoml::local_time>().get()->get();
            return QVariant(QTime(localTime.hour,
                                  localTime.minute,
                                  localTime.second,
                                  localTime.microsecond)
                            );
        } else if (base->as<cpptoml::local_date>().get()){
            auto localDate = base->as<cpptoml::local_date>().get()->get();
            return QVariant(QDate(localDate.year,
                                  localDate.month,
                                  localDate.day)
                            );
        } else if (base->as<cpptoml::local_datetime>().get()){
            auto localDateTime = base->as<cpptoml::local_datetime>().get()->get();
            return QVariant(QDateTime(
                            QDate(localDateTime.year,
                                  localDateTime.month,
                                  localDateTime.day),
                            QTime(localDateTime.hour,
                                  localDateTime.minute,
                                  localDateTime.second,
                                  localDateTime.microsecond)
                            ));
        } else if (base->as<cpptoml::offset_datetime>().get()){
            auto localDateTime = base->as<cpptoml::offset_datetime>().get()->get();
            return QVariant(QDateTime(
                            QDate(localDateTime.year,
                                  localDateTime.month,
                                  localDateTime.day),
                            QTime(localDateTime.hour,
                                  localDateTime.minute,
                                  localDateTime.second,
                                  localDateTime.microsecond),
                            QTimeZone(localDateTime.hour_offset * 3600 +
                                      localDateTime.minute_offset  * 60)
                            ));
        } else {
            return QVariant();
        }
    } else if (base->is_table()) {
        return deserialise(base->as_table(), parsingMap);
    } else if (base->is_table_array() && base->as_table_array()) {
        auto subresult = QVariantList{};
        auto tableArray = base->as_table_array();
        for (auto &iter : *tableArray) {
            subresult.append(deserialise(iter, parsingMap));
        }
        return QVariant(subresult);
    } else if (base->is_array() && base->as_array()) {
        auto subresult = QVariantList{};
        auto tableArray = base->as_array();
        for (auto &iter : *tableArray) {
            subresult.append(deserialise(iter, parsingMap));
        }
        return QVariant(subresult);
    }
    return QVariant();
}

QVariant deserialise(const std::shared_ptr<cpptoml::table> &table, const ParsingMap &parsingMap)
{
    if (table->is_table() && table->as_table()) {
        auto subresult = QVariantMap{};
        auto tableArray = table->as_table();
        for (auto &iter : *tableArray) {
            subresult.insert(decodeString(iter.first.c_str(), parsingMap), deserialise(iter.second, parsingMap));
        }
        return QVariant(subresult);
    } else {
        return QVariant();
    }
}

TomlReader::TomlReader(QString path, QObject *parent, bool enableParsingMap) :
    DictReader(path, parent),
    m_parsingMapEnabled(enableParsingMap)
{
}

void TomlReader::onPathSet()
{
    m_config = initTable(path());
    m_parsingMap = m_parsingMapEnabled ? getParsingMap() : ParsingMap{};
}

QVariantMap Settings::TomlReader::parse()
{
    return deserialise(initTable(path() + ".toml"), m_parsingMap).toMap();
}
