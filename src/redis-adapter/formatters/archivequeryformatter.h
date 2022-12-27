#ifndef ARCHIVEQUERYFORMATTER_H
#define ARCHIVEQUERYFORMATTER_H

#include <QObject>
#include "jsondict/jsondict.hpp"

class RADAPTER_SHARED_SRC ArchiveQueryFormatter : public QObject
{
    Q_OBJECT
public:
    explicit ArchiveQueryFormatter(const JsonDict &redisStreamEntry, QObject *parent = nullptr);

    QVariantList toWriteRecordsList(const JsonDict &keyVaultEntries) const;
signals:

private:
    JsonDict m_streamEntry;
};

#endif // ARCHIVEQUERYFORMATTER_H
