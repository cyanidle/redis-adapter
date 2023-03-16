#ifndef TOML_ADAPTER_H
#define TOML_ADAPTER_H

#include "../../dictreader.h"
#include "cpptoml.h"

namespace Settings {

class RADAPTER_SHARED_SRC TomlReader : public DictReader
{
    Q_OBJECT
public:
    TomlReader(const QString &dir, const QString &file, QObject *parent, bool enableParsingMap = true);
    QVariant getAll() override;
private:

    bool m_parsingMapEnabled;
    ParsingMap m_parsingMap;
};

}

#endif
