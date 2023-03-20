#ifndef LOCALSTORAGE_H
#define LOCALSTORAGE_H

#include <QObject>
#include <QSettings>
#include <QDateTime>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include "radapterlogging.h"

class RADAPTER_API LocalStorage : public QObject
{
    Q_OBJECT
public:
    static LocalStorage* instance();
    static void init(QObject *parent);

    QString currentDirectory()const;
    QString getStorageName()const;
    bool setStorageDirectory(const QString &path);
    bool setStorageName(const QString &name);

    QString getLastStreamId(const QString &streamKey);
    void setLastStreamId(const QString &streamKey, const QString &lastId);

    QVariant getValue(const QString &path, const bool forceFileRead = false);
    void setValue(const QString &path, const QVariant &value);
signals:

public slots:

private:
    explicit LocalStorage(QObject *parent = nullptr);
    static LocalStorage* prvInstance(QObject *parent = nullptr);

    QString getFullName()const;
    QString getFullName(const QString &wantedFile)const;

    QVariantMap m_cache;

    QString m_storageName;
    QString m_storageDirectory;
};

#endif // LOCALSTORAGE_H
