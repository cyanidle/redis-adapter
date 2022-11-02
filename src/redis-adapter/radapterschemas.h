#ifndef RADAPTER_SCHEMAS_H
#define RADAPTER_SCHEMAS_H

#include "radapter-broker/jsonschema.h"

namespace Radapter {

class AcknowledgeSchema : public Radapter::JsonSchema
{
    Q_OBJECT
public:
    static const AcknowledgeSchema *instance() {
        if (!m_instance) {
            m_instance = new AcknowledgeSchema;
        }
        return m_instance;
    }

    virtual Formatters::JsonDict prepareMsg(const QVariant &source = QVariant()) const override;
    virtual QVariant receive(const Formatters::JsonDict &source) const override;
    Formatters::JsonDict receiveAckJson(const Formatters::JsonDict &source) const {return receive(source).toMap();}
private:
    AcknowledgeSchema() = default;
    static AcknowledgeSchema * m_instance;

};


class RequestJsonSchema : public Radapter::JsonSchema
{
    Q_OBJECT
public:
    static const RequestJsonSchema *instance() {
        if (!m_instance) {
            m_instance = new RequestJsonSchema;
        }
        return m_instance;
    }

    virtual Formatters::JsonDict prepareMsg(const QVariant &source = QVariant()) const override;
    virtual QVariant receive(const Formatters::JsonDict &source) const override;
    bool isJsonRequested(const Formatters::JsonDict &source) const {return receive(source).toBool();}
private:
    RequestJsonSchema() = default;
    static RequestJsonSchema * m_instance;
};

class RequestKeysSchema : public Radapter::JsonSchema
{
    Q_OBJECT
public:
    static const RequestKeysSchema *instance() {
        if (!m_instance) {
            m_instance = new RequestKeysSchema;
        }
        return m_instance;
    }

    virtual Formatters::JsonDict prepareMsg(const QVariant &source = QVariant()) const override;
    virtual QVariant receive(const Formatters::JsonDict &source) const override;
    QStringList receiveKeys(const Formatters::JsonDict &source) const {return receive(source).toStringList();}
private:
    RequestKeysSchema() = default;
    static RequestKeysSchema * m_instance;

};




} // namespace Radapter

#endif // RADAPTER_SCHEMAS_H
