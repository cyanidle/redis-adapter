#include "validator_fetch.h"
#include "qmutex.h"
#include "radapterlogging.h"
#include "templates/algorithms.hpp"
#include <QMetaProperty>
#include <QString>

Validator::Fetched::Fetched(const QString &name) :
    m_executor(Private::fetchImpl(name)),
    m_name(name)
{

}

Validator::Fetched::Fetched(const Fetched &other) :
    m_executor(other.m_executor->newCopy()),
    m_name(other.m_name)
{

}

Validator::Fetched &Validator::Fetched::operator=(const Fetched &other)
{
    if (this == &other) return *this;
    m_executor.reset(other.m_executor->newCopy());
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

