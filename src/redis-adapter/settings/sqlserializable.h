#ifndef SQLSERIALIZABLE_H
#define SQLSERIALIZABLE_H

#include "settings-parsing/serializer.hpp"
#include "lib/mysql/mysqlclient.h"

template <typename Mixin>
class SqlSerializable : public Serializer::Serializable<Mixin>
{
public:
    explicit SqlSerializable(QObject *parent = nullptr);
};

#endif // SQLSERIALIZABLE_H
