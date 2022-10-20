#ifndef REDISQUERYFORMATTER_H
#define REDISQUERYFORMATTER_H

#include <QObject>
#include "json-formatters/formatters/dict.h"
#include "json-formatters/formatters/list.h"

class RADAPTER_SHARED_SRC RedisQueryFormatter : public QObject
{
    Q_OBJECT
public:
    explicit RedisQueryFormatter(const Formatters::Dict &jsonDict = Formatters::Dict{}, QObject *parent = nullptr);

    QString toAddStreamCommand(const QString &streamKey, quint32 maxLen = 0u) const;
    QString toTrimCommand(const QString &streamKey, quint32 maxLen) const;
    QString toReadStreamCommand(const QString &streamKey, qint32 blockTimeout, const QString &lastId = QString{}) const;
    QString toReadGroupCommand(const QString &streamKey,
                               const QString &groupName,
                               const QString &consumerName,
                               qint32 blockTimeout,
                               const QString &lastId = QString{}) const;
    QString toReadStreamAckCommand(const QString &streamKey, const QString &groupName, const QStringList &idList);
    QString toCreateGroupCommand(const QString &streamKey, const QString &groupName, const QString &startId = QString{});

    QString toGetIndexCommand(const QString &indexKey) const;
    QString toUpdateIndexCommand(const QString &indexKey) const;

    QString toMultipleGetCommand(const Formatters::List &keysList) const;
    QString toMultipleSetCommand() const;

    QString toKeyEventsSubscribeCommand(const QStringList &eventTypes) const;
    QString toPatternSubscribeCommand(const Formatters::List &patternList) const;
signals:

private:
    Formatters::Dict flatten() const;
    QString toEntryFields() const;
    QString toKeysFields(const Formatters::List &keysList) const;

    Formatters::Dict m_json;
};

#endif // REDISQUERYFORMATTER_H
