#ifndef RADAPTER_ACKNOWLEDGESCHEME_H
#define RADAPTER_ACKNOWLEDGESCHEME_H

#include "radapter-broker/jsonscheme.h"

namespace Radapter {

class AcknowledgeScheme : public Radapter::JsonScheme
{
    Q_OBJECT
public:
    static const AcknowledgeScheme *instance() {
        if (!m_instance) {
            m_instance = new AcknowledgeScheme;
        }
        return m_instance;
    }

    virtual Formatters::JsonDict prepareMsg(const QVariant &source = QVariant()) const override;
    virtual QVariant receive(const Formatters::JsonDict &source) const override;
private:
    AcknowledgeScheme() = default;
    static AcknowledgeScheme * m_instance;

};


class RequestJsonCommand : public Radapter::JsonScheme
{
    Q_OBJECT
public:
    static const RequestJsonCommand *instance() {
        if (!m_instance) {
            m_instance = new RequestJsonCommand;
        }
        return m_instance;
    }

    virtual Formatters::JsonDict prepareMsg(const QVariant &source = QVariant()) const override;
    virtual QVariant receive(const Formatters::JsonDict &source) const override;
private:
    RequestJsonCommand() = default;
    static RequestJsonCommand * m_instance;
};

class RequestKeysScheme : public Radapter::JsonScheme
{
    Q_OBJECT
public:
    static const RequestKeysScheme *instance() {
        if (!m_instance) {
            m_instance = new RequestKeysScheme;
        }
        return m_instance;
    }

    virtual Formatters::JsonDict prepareMsg(const QVariant &source = QVariant()) const override;
    virtual QVariant receive(const Formatters::JsonDict &source) const override;
private:
    RequestKeysScheme() = default;
    static RequestKeysScheme * m_instance;

};




} // namespace Radapter

#endif // RADAPTER_ACKNOWLEDGESCHEME_H
