#include "localstorage.h"

#define DEFAULT_STORAGE_NAME "localstorage.ini"
#define DEFAULT_STORAGE_DIRECTORY "conf"

#define LAST_STREAM_ID "last_id"

LocalStorage::LocalStorage(QObject *parent)
    : QObject(parent),
      m_cache{},
      m_storageName(DEFAULT_STORAGE_NAME)
{
    if (QDir::isAbsolutePath(DEFAULT_STORAGE_DIRECTORY)) {
        m_storageDirectory = DEFAULT_STORAGE_DIRECTORY;
    } else {
        auto storageDirectoryPath = QDir::toNativeSeparators("%1/%2")
                .arg(QCoreApplication::applicationDirPath(),DEFAULT_STORAGE_DIRECTORY);
        m_storageDirectory = QDir::toNativeSeparators(storageDirectoryPath);
    }
}

LocalStorage *LocalStorage::prvInstance(QObject *parent)
{
    static LocalStorage storage(parent);
    return &storage;
}

QString LocalStorage::currentDirectory() const
{
    return m_storageDirectory;
}

QString LocalStorage::getStorageName() const
{
    return m_storageName;
}

bool LocalStorage::setStorageDirectory(const QString &path)
{
    QDir oldDir(QCoreApplication::applicationDirPath());
    QDir newDir(QCoreApplication::applicationDirPath());
    if (QDir::isAbsolutePath(path)){
        newDir.setPath(path);
    }
    else {
        newDir.cd(path);
    }
    if (newDir==oldDir){
        reDebug() << QString("Localstorage: Directory change failed (cannot cd to %1)").arg(path);
        return false;
    }
    if (!newDir.exists()){
        reDebug() << QString("Localstorage: Directory change failed (%1 doesnt exist)").arg(path);
        return false;
    }
    if (!newDir.isReadable()){
        reDebug() << QString("Localstorage: Directory change failed (%1 unreadable)").arg(path);
        return false;
    }
    if (newDir.isEmpty()){
        reDebug() << QString("Localstorage: Directory change failed (%1 is empty)").arg(path);
        return false;
    }
    m_storageDirectory = newDir.canonicalPath();
    m_cache.clear();
    return true;
}

bool LocalStorage::setStorageName(const QString &name)
{
    m_storageName = name;
    m_cache.clear();
    return true;
}

LocalStorage *LocalStorage::instance()
{
    return prvInstance();
}

void LocalStorage::init(QObject *parent)
{
    prvInstance(parent);
}

QString LocalStorage::getLastStreamId(const QString &streamKey)
{
    auto key = QString("%1/%2").arg(streamKey, LAST_STREAM_ID);
    auto lastStreamId = getValue(key).toString();
    return lastStreamId;
}

QString LocalStorage::getFullName() const
{
    auto fullName =  QDir::toNativeSeparators("%1/%2").arg(m_storageDirectory, m_storageName);
    return fullName;
}

QString LocalStorage::getFullName(const QString &wantedName) const
{
    auto fullName =  QDir::toNativeSeparators("%1/%2").arg(m_storageDirectory, wantedName);
    return fullName;
}

void LocalStorage::setLastStreamId(const QString &streamKey, const QString &lastId)
{
    auto key = QString("%1/%2").arg(streamKey, LAST_STREAM_ID);
    setValue(key, lastId);
}

QVariant LocalStorage::getValue(const QString &key, const bool forceFileRead)
{
    QVariant storageValue;
    if (m_cache.contains(key) && !forceFileRead) {
        storageValue = m_cache.value(key);
    } else {
        QSettings localStorage(getFullName(), QSettings::IniFormat);
        storageValue = localStorage.value(key);
        m_cache[key] = storageValue;
    }
    return storageValue;
}

void LocalStorage::setValue(const QString &key, const QVariant &value)
{
    QSettings localStorage(getFullName(), QSettings::IniFormat);
    localStorage.setValue(key, value);
    m_cache[key] = value;
}
