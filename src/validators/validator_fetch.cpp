#include "validator_fetch.h"
#include "qmutex.h"
#include "radapterlogging.h"
#include <QMetaProperty>
#include <QString>

template <typename T>
using QStringMap = QMap<QString, T>;
Q_GLOBAL_STATIC(QStringMap<Validator::Function>, allValidators)
Q_GLOBAL_STATIC(QStringMap<QVariantList>, allArgs)
Q_GLOBAL_STATIC(QRecursiveMutex, staticMutex)

Validator::Function Validator::fetchFunction(const QLatin1String &name)
{
    return allValidators->value(QString(name).toLower());
}

int Validator::Private::add(Function func, const char *alias)
{
    QMutexLocker lock(&(*staticMutex));
    if (!qstrlen(alias)) return 0;
    if (allValidators->contains(QLatin1String(alias))) {
        throw std::runtime_error(std::string("Duplicate validator with name: ") + alias);
    }
    allValidators->insert(QLatin1String(alias), func);
    return 0;
}

Validator::Function Validator::fetchFunction(const char *name)
{
    return fetchFunction(QLatin1String(name));
}

Validator::Function Validator::fetchFunction(const QString &name)
{
    return allValidators->value(name.toLower());
}

Validator::Fetched::Fetched() :
    m_executor(nullptr)
{
}

Validator::Fetched::Fetched(const QString &name) :
    m_name(name),
    m_args(allArgs->value(name)),
    m_executor(fetchFunction(name))
{
    if (!m_executor && !name.isEmpty()) {
        throw std::runtime_error("Unavailable validator: " + name.toStdString());
    }
    auto test = QVariant{};
    validate(test);
}

void Validator::Fetched::addArgsFor(const QString &name, const QVariantList &args, const QString &newName)
{
    QMutexLocker lock(&(*staticMutex));
    if (allArgs->contains(newName)) {
        throw std::runtime_error("Args already added for: " + name.toStdString());
    }
    allArgs->insert(newName, args);
    if (allValidators->contains(newName)) {
        throw std::runtime_error("Validator name already taken: " + newName.toStdString());
    }
    allValidators->insert(newName, fetchFunction(name));
}

void Validator::Fetched::initialize()
{
    QMutexLocker lock(&(*staticMutex));
    auto converter = [](const QString& name){
        return Fetched(name);
    };
    auto backConverter = [](const Fetched& conv){
        return conv.m_executor ? allValidators->key(conv.m_executor) : "<Unkown>";
    };
    if (!QMetaType::registerConverter<QString, Fetched>(converter)) {
        throw std::runtime_error("Could not register Validator converter!");
    }
    if (!QMetaType::registerConverter<Fetched, QString>(backConverter)) {
        throw std::runtime_error("Could not register Validator back converter!");
    }
}

bool Validator::Fetched::validate(QVariant &target) const
{
    if (!m_executor) {
        throw std::runtime_error("Attempt to validate with nonexistent validator. Name: " + m_name.toStdString());
    }
    auto nullState = QVariant{};
    return m_executor(target, m_args, nullState);
}

bool Validator::Fetched::validate(QVariant &target, QVariant& state) const
{
    if (!m_executor) {
        throw std::runtime_error("Attempt to validate with nonexistent validator. Name: " + m_name.toStdString());
    }
    return m_executor(target, m_args, state);
}

const QString &Validator::Fetched::name() const
{
    return m_name;
}

bool Validator::Fetched::isValid() const
{
    return m_executor != nullptr;
}

bool Validator::Fetched::operator<(const Fetched &other) const
{
    return m_name < other.m_name;
}

bool Validator::Fetched::operator==(const QVariant &variant) const
{
    return m_name == variant.toString();
}

bool Validator::Fetched::operator!=(const QVariant &variant) const
{
    return !(*this)==variant;
}

Validator::Fetched::operator bool() const
{
    return isValid();
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
