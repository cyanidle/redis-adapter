#ifndef REDISQUERYFORMATTER_H
#define REDISQUERYFORMATTER_H

#include <QObject>
#include "jsondict/jsondict.hpp"

class RADAPTER_SHARED_SRC RedisQueryFormatter : public QObject
{
    Q_OBJECT
public:
    explicit RedisQueryFormatter(const JsonDict &jsonDict = JsonDict{}, QObject *parent = nullptr);

    QString toAddStreamCommand(const QString &streamKey, quint32 maxLen = 0u) const;
    QString toTrimCommand(const QString &streamKey, quint32 maxLen) const;
    static QString toReadStreamCommand(const QString &streamKey, const qint32 count, qint32 blockTimeout, const QString &lastId = QString{});
    static QString toReadGroupCommand(const QString &streamKey,
                               const QString &groupName,
                               const QString &consumerName,
                               qint32 blockTimeout,
                               const QString &lastId = QString{});
    static QString toReadStreamAckCommand(const QString &streamKey, const QString &groupName, const QStringList &idList);
    static QString toCreateGroupCommand(const QString &streamKey, const QString &groupName, const QString &startId = QString{});
    QString toUpdateIndexCommand(const QString &indexKey) const;
    QString toMultipleSetCommand() const;

    QString toKeyEventsSubscribeCommand(const QStringList &eventTypes) const;
    QString toPatternSubscribeCommand(const QStringList &patternList) const;
signals:

private:
    JsonDict flatten() const;
    QString toEntryFields() const;
    static QString toKeysFields(const QStringList &keysList);

    JsonDict m_json;
};

#endif // REDISQUERYFORMATTER_H
