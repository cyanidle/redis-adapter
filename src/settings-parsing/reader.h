#ifndef SETTINGS_READER_H
#define SETTINGS_READER_H

#include <QObject>
#include <QVariant>

namespace Settings {

class RADAPTER_SHARED_SRC Reader : public QObject
{
    Q_OBJECT
public:
    Reader(const QString &resource, const QString &path, QObject* parent = nullptr);

    QVariant operator[](const QString &key);
    virtual QVariant get(const QString &key) = 0;
    virtual QVariant getAll() = 0;

    const QString &resource() const;
    const QString &path() const;
    virtual void setResource(const QString path);
    virtual void setPath(const QString &path);
private:
    QString m_resource;
    QString m_path;
};

} // namespace Settings

#endif // SETTINGS_READER_H
