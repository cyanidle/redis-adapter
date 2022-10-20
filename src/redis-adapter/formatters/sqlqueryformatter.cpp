#include "sqlqueryformatter.h"

SqlQueryFormatter::SqlQueryFormatter(QObject *parent) : QObject(parent)
{
}

QString SqlQueryFormatter::toRegExpFilter(const QString &fieldName, const QStringList &keys)
{
    auto query = QString("WHERE %1 REGEXP '%2'").arg(fieldName, keys.join("|"));
    return query;
}
