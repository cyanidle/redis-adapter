#ifndef TOML_ADAPTER_H
#define TOML_ADAPTER_H

#include "../../dictreader.h"
#include "cpptoml.h"

namespace Settings {

class RADAPTER_SHARED_SRC TomlReader : public DictReader
{
    Q_OBJECT
public:
    TomlReader(QString path, QObject *parent, bool enableParsingMap = true);
    void onPathSet() override;
    QVariantMap parse() override;
private:
    ParsingMap getParsingMap();

    bool m_parsingMapEnabled;
    ParsingMap m_parsingMap;
    std::shared_ptr<cpptoml::table> m_config;
};

}

#endif
