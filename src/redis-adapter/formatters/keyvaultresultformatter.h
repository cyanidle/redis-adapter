#ifndef KEYVAULTRESULTFORMATTER_H
#define KEYVAULTRESULTFORMATTER_H

#include <QObject>
#include "jsondict/jsondict.hpp"

class RADAPTER_SHARED_SRC KeyVaultResultFormatter : public QObject
{
    Q_OBJECT
public:
    explicit KeyVaultResultFormatter(const QVariantList &sqlRecordsList = QVariantList{}, QObject *parent = nullptr);

    JsonDict toJsonEntries() const;
    bool isValid(const JsonDict &keyVaultEntry) const;

signals:

private:
    QVariantList m_sqlRecordsList;
};

#endif // KEYVAULTRESULTFORMATTER_H
