#ifndef SQLQUERYFORMATTER_H
#define SQLQUERYFORMATTER_H

#include <QObject>

class RADAPTER_SHARED_SRC SqlQueryFormatter : public QObject
{
    Q_OBJECT
public:
    explicit SqlQueryFormatter(QObject *parent = nullptr);

    QString toRegExpFilter(const QString &fieldName, const QStringList &keys);

signals:

};

#endif // SQLQUERYFORMATTER_H
