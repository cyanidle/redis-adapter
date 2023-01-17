#include "mysqlserializable.h"


SqlSerializable::SqlSerializable(MySql::Client *dbClient) :
    m_dbClient(dbClient)
{
    auto structure = JsonDict{structureUncached()};
    for (auto iter : structure) {
        if (iter.key().contains(LIST_MARKER) ||
            iter.key().contains(MAP_MARKER)) {
            throw std::invalid_argument("MAP and CONTAINER are unsupported!");
        }
    }
}

bool SqlSerializable::tryFetch()
{
    if (!checkConnection()) {
        return false;
    }
#error "Finish Sql Fetch"
}

bool SqlSerializable::trySave()
{
    if (!checkConnection()) {
        return false;
    }
#error "Implement Sql Save"
}

bool SqlSerializable::checkConnection()
{
    bool ok = true;
    if (!m_dbClient->isOpened()) {
        ok = m_dbClient->open();
    }
    return ok;
}

SqlQObject::SqlQObject(MySql::Client *dbClient, QObject *parent) :
    QObjectMixin(dbClient, parent)
{}

SqlGadget::SqlGadget(MySql::Client *dbClient) :
    GadgetMixin(dbClient)
{}
