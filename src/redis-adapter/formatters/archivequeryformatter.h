#ifndef ARCHIVEQUERYFORMATTER_H
#define ARCHIVEQUERYFORMATTER_H

#include <QObject>
#include "JsonFormatters"

class RADAPTER_SHARED_SRC ArchiveQueryFormatter : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveQueryFormatter(const Formatters::Dict &redisStreamEntry, QObject *parent = nullptr);

    Formatters::List toWriteRecordsList(const Formatters::Dict &keyVaultEntries) const;
signals:

private:
    Formatters::Dict m_streamEntry;
};

#endif // ARCHIVEQUERYFORMATTER_H
