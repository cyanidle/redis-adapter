#ifndef TOMLADAPTER_H
#define TOMLADAPTER_H

#include "private/global.h"
#include <QObject>
#include "cpptoml.h"
#include <QVariantMap>

namespace Settings {

class RADAPTER_SHARED_SRC TomlReader
{
public:
    static QVariantMap parse(const QString &path, const ParsingMap& parsingMap = {});
    static ParsingMap getParsingMap();
private:
    static std::shared_ptr<cpptoml::table> initTable(const QString &path);
    static QVariant deserialise(const std::shared_ptr<cpptoml::base> &base, const ParsingMap &parsingMap);
    static QVariant deserialise(const std::shared_ptr<cpptoml::table> &table, const ParsingMap &parsingMap);
    static QString decodeString(const QString &encodedString, const ParsingMap &parsingMap);
};

}

#endif // FILEREADER_H
