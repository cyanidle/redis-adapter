#ifndef STREAMENTRIESMAPFORMATTER_H
#define STREAMENTRIESMAPFORMATTER_H

#include <QObject>
#include "json-formatters/formatters/dict.h"
#include "json-formatters/formatters/list.h"

class StreamEntriesMapFormatter : public QObject
{
    Q_OBJECT
public:
    explicit StreamEntriesMapFormatter(const Formatters::Dict &streamEntriesJson, QObject *parent = nullptr);

    static bool isValid(const Formatters::Dict &streamEntriesJson);
    Formatters::List toEntryList();
    Formatters::Dict joinToLatest();
signals:
private:
    Formatters::Dict m_streamEntriesMap;

};

#endif // STREAMENTRIESMAPFORMATTER_H
