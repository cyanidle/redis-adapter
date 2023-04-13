#include "validator_fetch.h"
#include "qmutex.h"
#include "radapterlogging.h"
#include <QMetaProperty>
#include <QString>

template <typename T>
using QStringMap = QMap<QString, T>;

Q_GLOBAL_STATIC(QStringMap<const Validator::Executor*>, allValidators)
Q_GLOBAL_STATIC(QRecursiveMutex, staticMutex)

Validator::Executor::Executor(Function func) :
    m_func(func)
{
}

bool Validator::Executor::validate(QVariant &target) const
{
    return m_func(target);
}

QString Validator::Executor::name() const
{
    QMutexLocker lock(&(*staticMutex));
    return allValidators->key(this);
}

QStringList Validator::Executor::aliases() const
{
    QMutexLocker lock(&(*staticMutex));
    return allValidators->keys(this);
}

const Validator::Executor *Validator::fetch(const QLatin1String &name)
{
    QMutexLocker lock(&(*staticMutex));
    return allValidators->value(QString(name).toLower());
}

int Validator::Private::add(Function func, const char *alias)
{
    QMutexLocker lock(&(*staticMutex));
    if (!qstrlen(alias)) return 0;
    if (allValidators->contains(QLatin1String(alias))) {
        throw std::runtime_error(std::string("Duplicate validator with name: ") + alias);
    }
    allValidators->insert(QLatin1String(alias), new Executor(func));
    return 0;
}

const Validator::Executor *Validator::fetch(const char *name)
{
    QMutexLocker lock(&(*staticMutex));
    return fetch(QLatin1String(name));
}

const Validator::Executor *Validator::fetch(const QString &name)
{
    QMutexLocker lock(&(*staticMutex));
    return allValidators->value(name.toLower());
}

QString Validator::nameOf(const Executor *validator)
{
    QMutexLocker lock(&(*staticMutex));
    return allValidators->key(validator);
}

Serializable::Validator::Validator() :
    m_executor(nullptr)
{
}

Serializable::Validator::Validator(const QString &name) :
    m_name(name),
    m_executor(nullptr)
{
    if (name.isEmpty()) return;
    auto fetched = ::Validator::fetch(name);
    if (!fetched) throw std::runtime_error("Unavailable validator: " + name.toStdString());
    m_executor = fetched;
}

void Serializable::Validator::initialize()
{
    auto converter = [](const QString& name){
        return Validator(name);
    };
    auto backConverter = [](const Validator& conv){
        return conv.m_executor ? conv.m_executor->name() : "";
    };
    if (!QMetaType::registerConverter<QString, Validator>(converter)) {
        throw std::runtime_error("Could not register Validator converter!");
    }
    if (!QMetaType::registerConverter<Validator, QString>(backConverter)) {
        throw std::runtime_error("Could not register Validator back converter!");
    }
}

bool Serializable::Validator::validate(QVariant &target) const
{
    if (!m_executor) {
        throw std::runtime_error("Attempt to validate with nonexistent validator. Name: " + m_name.toStdString());
    }
    return m_executor->validate(target);
}

const QString &Serializable::Validator::name() const
{
    return m_name;
}

bool Serializable::Validator::operator<(const Validator &other) const
{
    return m_name < other.m_name;
}

bool Serializable::Validator::operator==(const QVariant &variant) const
{
    return m_executor ? m_executor->name() == variant.toString() : false;
}

bool Serializable::Validator::operator!=(const QVariant &variant) const
{
    return !(*this)==variant;
}

Serializable::Validator::operator bool() const
{
    return m_executor;
}

int Validator::Private::add(Function func, const char **aliases, int count)
{
    for (int i = 0; i < count; ++i) {
        add(func, aliases[i]);
    }
    return 0;
}

void Validator::makeFetchable(Function validatingFunction, const QStringList &aliases)
{
    Private::add(validatingFunction, aliases);
}

int Validator::Private::add(Function func, const QStringList &aliases)
{
    for (const auto &alias: aliases) {
        add(func, alias.toStdString().c_str());
    }
    return 0;
}

const QStringList Validator::available()
{
    return allValidators->keys();
}
