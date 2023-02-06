#ifndef FILEREADER_H
#define FILEREADER_H

#include "private/global.h"
#include <QObject>
#include "cpptoml.h"
#include <QVariantMap>

namespace Settings {

typedef QMap<QString /*encoded string*/, QString /*decoded value*/> ParsingMap;


class RADAPTER_SHARED_SRC FileReader : public QObject
{
    Q_OBJECT
public:
    explicit FileReader(const QString &filepath, QObject *parent);
    bool setPath(const QString &path);
    bool initParsingMap();
    QVariant deserialise(const QString &key = {}, bool useParsingMap = true);
    const QString &filePath() const {return m_filepath;}
signals:

public slots:

private:
    ParsingMap parsingFromMap(const QVariantMap &src);
    void initTable();
    ParsingMap getParsingMap();
    static QVariant deserialise(const std::shared_ptr<cpptoml::base> &base, const ParsingMap &parsingMap);
    static QVariant deserialise(const std::shared_ptr<cpptoml::table> &table, const ParsingMap &parsingMap);
    static QString decodeString(const QString &encodedString, const ParsingMap &parsingMap);

    std::shared_ptr<cpptoml::table> m_config;
    QString m_filepath;
    ParsingMap m_parsingMap;
};

}

#endif // FILEREADER_H
