#ifndef ROUTESPROVIDER_H
#define ROUTESPROVIDER_H

#include "jsonroute.h"
#include "private/global.h"
#include <QMutex>

class RADAPTER_API JsonRoutesProvider : public QObject
{
    Q_OBJECT
public:
    static JsonRoutesProvider* instance() {
        return &prvInstance();
    }
    static void init(const JsonRoute::Map &bindings, QObject* parent = nullptr) {
        prvInstance(bindings, parent);
    }
    const JsonRoute &getBinding(const QString &name);
private:
    static JsonRoutesProvider &prvInstance(const JsonRoute::Map &bindings = JsonRoute::Map(), QObject* parent = nullptr) {
        static JsonRoutesProvider provider(bindings, parent);
        return provider;
    }
    JsonRoutesProvider(const JsonRoute::Map &bindings, QObject* parent = nullptr);
    JsonRoute::Map m_bindings;
};

#endif // ROUTESPROVIDER_H
