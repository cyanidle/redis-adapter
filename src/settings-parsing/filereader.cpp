#include "filereader.h"
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>
#include "qtimezone.h"
#include "serializer.hpp"

using namespace Settings;

typedef quint16 MbRegister_t;

ParsingMap FileReaderParsingFromMap(const QVariantMap &src) {
    return Serializer::convertQMap<QString>(src);
}

FileReader::FileReader(const QString &filepath, QObject *parent)
    : QObject(parent),
      m_config(nullptr),
      m_filepath(filepath)
{
}

void FileReader::initTable()
{
    try {
        auto path = QCoreApplication::applicationDirPath() + "/" + m_filepath;
        auto nativePath = QDir::toNativeSeparators(path);
        if (!QFile(QDir(nativePath).absolutePath()).exists()) {
            path = QDir::currentPath()  + "/" + m_filepath;
            nativePath = QDir::toNativeSeparators(path);
        }
        m_config = cpptoml::parse_file(nativePath.toStdString());
    } catch (const cpptoml::parse_exception& e) {
        settingsParsingWarn() << QString("settings: Failed to parse %1: ")
                     .arg(m_filepath)
                  << e.what();
        m_config = nullptr;
    }
}

bool FileReader::setPath(const QString &path)
{
    m_config = nullptr;
    m_filepath.clear();
    QDir wantedPath(
                QDir::toNativeSeparators(
                    QCoreApplication::applicationDirPath() + "/" + path));
    if (QFile(wantedPath.absolutePath()).exists()) {
        settingsParsingWarn() << "Settings path to: " << path;
        m_filepath = path;
        return true;
    } else {
        wantedPath.setPath(
                    QDir::toNativeSeparators(
                        QDir::currentPath() + "/" + path));
        if (QFile(wantedPath.absolutePath()).exists()) {
            m_filepath = path;
            return true;
        }
    }
    settingsParsingWarn() << "Filereader: Could not set path to: " << path;
    m_filepath.clear();
    return false;
}

ParsingMap FileReader::getParsingMap()
{
    if (!m_parsingMap.isEmpty()) {
        return m_parsingMap;
    }
    auto rawParsingTable = deserialise("parsing", false).toMap();
    auto result =  ParsingMap{};
    for (auto parsingItem = rawParsingTable.begin();
         parsingItem != rawParsingTable.end();
         parsingItem++)
    {
        result.insert(parsingItem.key(), parsingItem.value().toString());
    }
    m_parsingMap = result;
    return result;
}

bool FileReader::initParsingMap()
{
    auto parsingMap = getParsingMap();
    return !parsingMap.isEmpty();
}

QVariant FileReader::deserialise(const std::shared_ptr<cpptoml::base> &base, const ParsingMap &parsingMap)
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

QVariant FileReader::deserialise(const std::shared_ptr<cpptoml::table> &table, const ParsingMap &parsingMap)
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

QVariant FileReader::deserialise(const QString &key, bool useParsingMap)
{
    if (m_filepath.isEmpty()) {
        settingsParsingWarn() << "Filereader: Attempt to deserialise (" << key << "), while filepath is empty!";
        return QVariant();
    }
    if (!m_config) {
        initTable();
    }
    if (!m_config) {
        return QVariant();
    }
    std::shared_ptr<cpptoml::base> target = std::shared_ptr<cpptoml::base>();
    try {
        if (key.isEmpty()) {
            target = m_config;
        } else {
            target = m_config->get_qualified(key.toStdString());
        }
    } catch (std::out_of_range const&){
        settingsParsingWarn() << "Filereader: (In file:" << m_filepath << ") No such key exists: " << key;
        return QVariant();
    }
    if (!target) {
        settingsParsingWarn() << "Filereader: (In file:" << m_filepath << ") Error getting table by the key: " << key;
        return QVariant();
    }
    if (useParsingMap) {
        auto parsingMap = getParsingMap();
        if (!parsingMap.isEmpty()) {
            return deserialise(target, parsingMap);
        }
    }
    return deserialise(target, ParsingMap());
}

QString FileReader::decodeString(const QString &encodedString, const ParsingMap &parsingMap)
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
