#include "validators_pr.h"
#include <QRegularExpression>
#include <QRecursiveMutex>
#include "private/parsing_private.h"
#include "settings-parsing/convertutils.hpp"

using namespace Validator;

Q_GLOBAL_STATIC(QRecursiveMutex, staticMutex)
Q_GLOBAL_STATIC(QStringMap<IFactory*>, all)
Q_GLOBAL_STATIC(QStringMap<Validator::IExecutor*>, cache)


void Validator::Private::registerImpl(IFactory *factory, const QStringList &names)
{
    QMutexLocker lock(&(*staticMutex));
    for (auto &name: names) {
        if (all->contains(name)) {
            delete factory;
            throw std::runtime_error("Name: "+name.toStdString()+" is already taken!");
        }
        all->insert(name, factory);
    }
}

Validator::IExecutor *Validator::Private::fetchImpl(const QString &rawName)
{
    QMutexLocker lock(&(*staticMutex));
    auto name = Radapter::Private::normalizeFunc(rawName);
    try{
        if (cache->contains(name)) {
            return cache->value(name)->newCopy();
        } else {
            auto [func, data] = Radapter::Private::parseFunc(name);
            if (!all->contains(func)) {
                throw std::runtime_error("Validator with name: ("+name.toStdString()+") does not exist!");
            }
            auto fac = all->value(func);
            auto res = fac->create(convertToQList(data));
            cache->insert(name, res);
            return res->newCopy();
        }
    } catch (std::exception &exc) {
        throw std::runtime_error("While fetching validator: "+name.toStdString()+" --> "+exc.what());
    }
}

IExecutor *Private::fetchImpl(const QString &name, const QVariantList &args)
{
    auto asStr = convertQList<QString>(args);
    return fetchImpl(name%'('%asStr.join(',')%')');
}
