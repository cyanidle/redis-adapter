#ifndef STREAMENTRIESMAPFORMATTER_H
#define STREAMENTRIESMAPFORMATTER_H

#include <QObject>
#include "jsondict/jsondict.h"

class StreamEntriesMapFormatter : public QObject
{
    Q_OBJECT
public:
    explicit StreamEntriesMapFormatter(const JsonDict &streamEntriesJson, QObject *parent = nullptr);

    static bool isValid(const JsonDict &streamEntriesJson);
    QVariantList toEntryList();
    JsonDict joinToLatest();
signals:
private:
    JsonDict m_streamEntriesMap;

};

#endif // STREAMENTRIESMAPFORMATTER_H
