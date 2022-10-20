#include "tst_redisqueryformatter.h"

#include <QVariant>
#include <QString>
#include "formatters/jsondict.h"

#define BLOCK_TIMEOUT_MS 30000


TestRedisQueryFormatter::TestRedisQueryFormatter() :
m_empty_formatter()
{
}


void TestRedisQueryFormatter::testToAddStreamCommand(){
  QVariantMap map{
    {"station:16:level:change_speed", QVariant(0.0135498046875)},
    {"station:16:level:readings:1", QVariant(2.736364)}
  };
  JsonDict json(map);
  QCOMPARE(
    RedisQueryFormatter(json).toAddStreamCommand("station:16"),
    "XADD station:16 * station:16:level:change_speed 0.0135498046875 station:16:level:readings:1 2.736364"
  );
}

void TestRedisQueryFormatter::testToTrimCommand(){
  QCOMPARE(m_empty_formatter.toTrimCommand("station:16", 1000000),
  QString("XTRIM station:16 MAXLEN ~ 1000000"));
}

void TestRedisQueryFormatter::testToReadStreamCommand(){
  QCOMPARE(
    RedisQueryFormatter{}.toReadStreamCommand("station:16:commands", BLOCK_TIMEOUT_MS, ""),
    QString("XREAD BLOCK 30000 STREAMS station:16:commands $")
  );
}

void TestRedisQueryFormatter::testToGetIndexCommand(){
  QCOMPARE(RedisQueryFormatter{}.toReadStreamCommand("station:16:commands", BLOCK_TIMEOUT_MS, ""),
  QString("XREAD BLOCK 30000 STREAMS station:16:commands $"));
}

void TestRedisQueryFormatter::testToUpdateIndexCommand(){
  JsonDict json(QVariantMap{
    {"station:16:level:change_speed", QVariant(0.009613037109375)},
    {"station:16:level:readings:1", QVariant(1.7673972845077515)}
  });
  QCOMPARE(RedisQueryFormatter(json).toUpdateIndexCommand("station:16:index"),
  QString("SADD station:16:index station:16:level:change_speed station:16:level:readings:1"));
}

void TestRedisQueryFormatter::testToMultipleGetCommand(){
  JsonList keys(QVariantList{
    QVariant("station:16:sensors:level_readings:1"),
    QVariant("station:16:command:sensors:remote_level:3"),
    QVariant("station:16:sensors:level_readings:3")
  });
  QCOMPARE(m_empty_formatter.toMultipleGetCommand(keys),
  QString("MGET station:16:sensors:level_readings:1 station:16:command:sensors:remote_level:3 station:16:sensors:level_readings:3"));
}

void TestRedisQueryFormatter::testToMultipleSetCommand(){
  JsonDict json(QVariantMap{
    {"station:16:level:change_speed",QVariant(0.01025390625)},
    {"station:16:level:readings:1",QVariant(2.880859375)}
  });
  QCOMPARE(RedisQueryFormatter(json).toMultipleSetCommand(),
  QString("MSET station:16:level:change_speed 0.01025390625 station:16:level:readings:1 2.880859375"));
}


QTEST_GUILESS_MAIN(TestRedisQueryFormatter)

