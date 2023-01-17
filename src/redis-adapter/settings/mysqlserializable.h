#ifndef MYSQLSERIALIZABLE_H
#define MYSQLSERIALIZABLE_H

#include "jsondict/jsondict.hpp"
#include "settings-parsing/serializer.hpp"
#include "lib/mysql/mysqlclient.h"

class SqlSerializable : protected Serializer::Serializable
{
public:
    explicit SqlSerializable(MySql::Client *dbClient);
    bool tryFetch();
    bool trySave();
private:
    bool checkConnection();
    MySql::Client *m_dbClient;
};

class SqlGadget : public Serializer::GadgetMixin<SqlSerializable>
{
    explicit SqlGadget(MySql::Client *dbClient);
};

class SqlQObject : public Serializer::QObjectMixin<SqlSerializable>
{
    Q_OBJECT
public:
    explicit SqlQObject(MySql::Client *dbClient, QObject *parent = nullptr);
};

#endif // MYSQLSERIALIZABLE_H
