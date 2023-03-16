#ifndef FILEREADER_H
#define FILEREADER_H

#include "private/global.h"
#include "reader.h"
#include <QObject>
#include <QVariantMap>

namespace Settings {

class RADAPTER_SHARED_SRC DictReader : public Reader
{
public:
    DictReader(const QString &resource, const QString &path, QObject *parent = nullptr);
    QVariant get(const QString &key) override;
    virtual void setResource(const QString path) override;
    virtual void setPath(const QString &path) override;
private:
    QVariantMap m_config;
};

}

#endif // FILEREADER_H
