#ifndef KEYVAULTRESULTFORMATTER_H
#define KEYVAULTRESULTFORMATTER_H

#include <QObject>
#include "json-formatters/formatters/list.h"

class RADAPTER_SHARED_SRC KeyVaultResultFormatter : public QObject
{
    Q_OBJECT
public:
    explicit KeyVaultResultFormatter(const Formatters::List &sqlRecordsList = Formatters::List{}, QObject *parent = nullptr);

    Formatters::Dict toJsonEntries() const;
    bool isValid(const Formatters::Dict &keyVaultEntry) const;

signals:

private:
    Formatters::List m_sqlRecordsList;
};

#endif // KEYVAULTRESULTFORMATTER_H
