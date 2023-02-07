#include "mysqlserializable.h"

SqlSerializable::SqlSerializable()
{}

MySql::QueryRecord SqlSerializable::fetchRecord()
{
    return {};
}

MySql::QueryRecord SqlSerializable::saveRecord()
{
    return {};
}

SqlQObject::SqlQObject(QObject *parent) :
    QObjectMixin(parent)
{}

SqlGadget::SqlGadget() :
    GadgetMixin()
{}
