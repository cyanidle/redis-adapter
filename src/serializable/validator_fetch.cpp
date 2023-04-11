#include "validator_fetch.h"
#include <QMetaProperty>
#include <QString>

template <typename T>
using QStringMap = QMap<QString, T>;
Q_GLOBAL_STATIC(QStringMap<const Validator::Executor*>, allValidators)

Validator::Executor::Executor(Function func) :
    m_func(func)
{
}

bool Validator::Executor::validate(QVariant &target) const
{
    return m_func(target);
}

const Validator::Executor *Validator::fetch(const QLatin1String &name)
{
    return allValidators->value(QString(name).toLower());
}

int Validator::Private::add(Function func, const char *name)
{
    if (!qstrlen(name)) return 0;
    if (allValidators->contains(QLatin1String(name))) {
        throw std::runtime_error(std::string("Duplicate validator with name: ") + name);
    }
    allValidators->insert(QLatin1String(name), new Executor(func));
    return 0;
}

const Validator::Executor *Validator::fetch(const char *name)
{
    return fetch(QLatin1String(name));
}

const Validator::Executor *Validator::fetch(const QString &name)
{
    return allValidators->value(name.toLower());
}

QString Validator::nameOf(const Executor *validator)
{
    return allValidators->key(validator);
}

bool Serializable::Validator::updateWithVariant(const QVariant &source) {
    auto asStr = source.toString();
    if (asStr.isEmpty()) return false;
    auto fetched = ::Validator::fetch(asStr);
    if (!fetched) return false;
    this->value = fetched;
    return true;
}

QVariant Serializable::Validator::readVariant() const {
    return QVariant::fromValue(::Validator::nameOf(this->value));
}

int Validator::Private::add(Function func, const char **names, int count)
{
    for (int i = 0; i < count; ++i) {
        add(func, names[i]);
    }
    return 0;
}
