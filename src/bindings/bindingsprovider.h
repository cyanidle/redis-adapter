#ifndef BINDINGSPROVIDER_H
#define BINDINGSPROVIDER_H

#include "jsonbinding.h"
#include "private/global.h"
#include <QMutex>

class RADAPTER_API BindingsProvider : public QObject
{
    Q_OBJECT
public:
    static BindingsProvider* instance() {
        return &prvInstance();
    }
    static void init(const JsonBinding::Map &bindings, QObject* parent = nullptr) {
        prvInstance(bindings, parent);
    }
    const JsonBinding &getBinding(const QString &name);
private:
    static BindingsProvider &prvInstance(const JsonBinding::Map &bindings = JsonBinding::Map(), QObject* parent = nullptr) {
        static BindingsProvider provider(bindings, parent);
        return provider;
    }
    BindingsProvider(const JsonBinding::Map &bindings, QObject* parent = nullptr);
    JsonBinding::Map m_bindings;
};

#endif // BINDINGSPROVIDER_H
