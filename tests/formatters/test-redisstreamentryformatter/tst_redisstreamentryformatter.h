#ifndef RSK_TEST_REDISSTREAMENTRYFORMATTER_H
#define RSK_TEST_REDISSTREAMENTRYFORMATTER_H
#include <QtTest>
#include "formatters/redisstreamentryformatter.h"

class TestRedisStreamEntryFormatter : public QObject
{
    Q_OBJECT
private slots:
    void TestEventTime_data();
    void TestEventTime();
    void TestEntryKeys_data();
    void TestEntryKeys();
    void TestEventDataDict_data();
    void TestEventDataDict();

};

#endif // RSK_TEST_REDISSTREAMENTRYFORMATTER_H
