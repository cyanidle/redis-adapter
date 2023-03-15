#ifndef FILEREADER_H
#define FILEREADER_H

#include "private/global.h"
#include <QObject>
#include <QVariantMap>

namespace Settings {

class Reader : public QObject
{
    Q_OBJECT
public:
    Reader(const QString &path, QObject* parent = nullptr);

    QVariant operator[](QString key);
    virtual QVariant get(QString key) = 0;

    const QString &path() const;
    void setPath(const QString &path);
protected:
    virtual void onPathSet() = 0;
private:
    QString m_path;
};

class DictReader : public Reader
{
public:
    DictReader(const QString &path, QObject *parent = nullptr);
    QVariant get(QString key) override;
protected:
    virtual QVariantMap parse() = 0;
private:
     QVariantMap m_config;
};

}

#endif // FILEREADER_H
