#ifndef STREAMENTRIESMAPFORMATTER_H
#define STREAMENTRIESMAPFORMATTER_H

#include <QObject>
#include "jsondict/jsondict.hpp"

class StreamEntriesMapFormatter
{
public:
    explicit StreamEntriesMapFormatter(const JsonDict &streamEntriesJson);

    static bool isValid(const JsonDict &streamEntriesJson);
    QVariantList toEntryList();
    JsonDict joinToLatest();
signals:
private:
    JsonDict m_streamEntriesMap;

};

#endif // STREAMENTRIESMAPFORMATTER_H
