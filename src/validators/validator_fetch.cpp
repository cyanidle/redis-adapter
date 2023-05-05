#include "validator_fetch.h"
#include "qmutex.h"
#include "radapterlogging.h"
#include "templates/algorithms.hpp"
#include <QMetaProperty>
#include <QString>

void Validator::Fetched::initializeVariantFetching()
{
    auto converter = [](const QString& name){
        return Fetched(name);
    };
    auto backConverter = [](const Fetched& conv){
        return conv.m_name;
    };
    if (!QMetaType::registerConverter<QString, Fetched>(converter)) {
        throw std::runtime_error("Could not register Validator converter!");
    }
    if (!QMetaType::registerConverter<Fetched, QString>(backConverter)) {
        throw std::runtime_error("Could not register Validator back converter!");
    }
}

Validator::Fetched::Fetched(const QString &name) :
    m_executor(Private::fetchImpl(name)),
    m_name(name)
{

}

Validator::Fetched::Fetched(const QString &name, const QVariantList &args) :
    m_executor(Private::fetchImpl(name, args)),
    m_name(name)
{

}

Validator::Fetched::Fetched(const Fetched &other) :
    m_executor(other.m_executor ? other.m_executor->newCopy() : nullptr),
    m_name(other.m_name)
{

}

Validator::Fetched &Validator::Fetched::operator=(const Fetched &other)
{
    if (this == &other) return *this;
    m_executor.reset(other.m_executor ? other.m_executor->newCopy() : nullptr);
    m_name = other.m_name;
    return *this;
}

Validator::Fetched &Validator::Fetched::operator=(Fetched &&other)
{
    return (*this) = other;
}

Validator::Fetched::Fetched(Fetched &&other) :
    Fetched(other)
{

}

bool Validator::Fetched::validate(QVariant &target) const
{
    if (!m_executor) {
        throw std::runtime_error("Attempt to validate with nonexistent validator. Name: " + m_name.toStdString());
    }
    return m_executor->validate(target);
}

const QString &Validator::Fetched::name() const
{
    return m_name;
}

bool Validator::Fetched::isValid() const
{
    return m_executor != nullptr;
}
Validator::Fetched::operator bool() const
{
    return isValid();
}

