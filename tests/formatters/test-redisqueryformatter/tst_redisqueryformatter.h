#ifndef RSK_TEST_REDISQUERYFORMATTER_H
#define RSK_TEST_REDISQUERYFORMATTER_H
#include <QtTest>
#include "formatters/redisqueryformatter.h"

class TestRedisQueryFormatter : public QObject
{
    Q_OBJECT
public:
    TestRedisQueryFormatter();
private slots:
    void testToAddStreamCommand();
    void testToTrimCommand();
    void testToReadStreamCommand();
    void testToGetIndexCommand();
    void testToUpdateIndexCommand();
    void testToMultipleGetCommand();
    void testToMultipleSetCommand();
private:
    RedisQueryFormatter m_empty_formatter;
};

#endif // RSK_TEST_REDISQUERYFORMATTER_H
