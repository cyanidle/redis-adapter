#ifndef MYSQLSERIALIZABLE_H
#define MYSQLSERIALIZABLE_H

#include "jsondict/jsondict.hpp"
#include "settings-parsing/serializer.hpp"
#include "lib/mysql/mysqlclient.h"

class SqlSerializable : protected Serializer::Serializable
{
public:
    explicit SqlSerializable();
    MySql::QueryRecord fetchRecord();
    MySql::QueryRecord saveRecord();
};

class SqlGadget : public Serializer::GadgetMixin<SqlSerializable>
{
    explicit SqlGadget();
};

class SqlQObject : public Serializer::QObjectMixin<SqlSerializable>
{
    Q_OBJECT
public:
    explicit SqlQObject(QObject *parent = nullptr);
};

#endif // MYSQLSERIALIZABLE_H
