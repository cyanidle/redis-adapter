#include "tst_redisstreamentryformatter.h"
#include "formatters/redisstreamentryformatter.h"
#include "rsktestmacros.h"

//#define RSK_TEST_TIMESTRING "2022-05-11T12:19:28.000"
//#define RSK_TEST_TIMESTAMP 1652264368U
//#define MILLISECONDS_FORMAT "yyyy-MM-ddThh:mm:ss.zzz"

void TestRedisStreamEntryFormatter::TestEventTime_data(){
    QTest::addColumn<JsonDict>("source");
    QTest::addColumn<QDateTime>("expected");
    ////// Basic test
    QVariantMap src_map0{
          {RSK_TEST_TIMESTRING, RSK_TEST_NEST_MAP_ONE_KEY(
              "station:16:command:network_rssi", RSK_TEST_STR_VARIANT("3"))}
        };
    JsonDict source0(src_map0);
    auto expected0 = QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT);
    QTest::newRow("Basic test") << source0 << expected0;
    ////// Wrong timestring
    QVariantMap src_map1{
          {"NotATimestring", RSK_TEST_NEST_MAP_ONE_KEY(
              "station:16:command:network_rssi", RSK_TEST_STR_VARIANT("3"))}
        };
    JsonDict source1(src_map1);
    auto expected1 = QDateTime::fromString("", MILLISECONDS_FORMAT);
    auto expected1_b = QDateTime();
    QTest::newRow("Wrong Timestring (Should return empty QDate)") << source1 << expected1;
    QTest::newRow("Wrong Timestring (Should return empty QDate)") << source1 << expected1_b;
    //QFAIL("(Тест проходит 13.05.22) Следует продумать поведение TimeFormatter в случае ошибочной строки времени (Пока что возвращается пустая дата)");
}

void TestRedisStreamEntryFormatter::TestEventTime(){
    QFETCH(JsonDict, source);
    QFETCH(QDateTime, expected);
    auto actual = RedisStreamEntryFormatter(source).eventTime();
    QCOMPARE(actual,expected);
}

void TestRedisStreamEntryFormatter::TestEntryKeys_data(){
    QTest::addColumn<JsonDict>("source");
    QTest::addColumn<QStringList>("expected_keys");
    ///////// 0: Basic test
    QVariantMap src_map0{
    {"TopKey0", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")}})
     }};
    JsonDict source0(src_map0);
    QStringList expected0{"TestKey0", "TestKey1"};
    QTest::newRow("Basic test") << source0 << expected0;
    ///////// 1: 2 lvls nesting
    QVariantMap src_map1_a{
    {"TopKey0", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")}})
     }};
    QVariantMap src_map1_b{
    {"TopKey1", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")}})
     }};
    JsonDict source1(QVariantMap{
                         {"TopTopKey0", QVariant(src_map1_a)},
                         {"TopTopKey1", QVariant(src_map1_b)}
                     });
    QStringList expected1{"TopKey0"};
    QTest::newRow("2 top level keys (first should be used)") << source1 << expected1;
    ///////// 2: Deeper nesting
    QVariantMap src_map2_a{
    {"TopKey0", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_NEST_MAP_ONE_KEY(
            "NestedKey0",RSK_TEST_STR_VARIANT("Value0"))},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")},
         {"TestKey2", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey3", RSK_TEST_STR_VARIANT("Value1")}})
     }};
    JsonDict source2(src_map2_a);
    QStringList expected2{"TestKey0","TestKey1","TestKey2","TestKey3"};
    QTest::newRow("Deeper nesting") << source2 << expected2;
    //
    QVariantMap src_map2_b{
    {"TopKey0", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_NEST_MAP_ONE_KEY(
            "NestedKey0",RSK_TEST_STR_VARIANT("Value0"))},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey3", RSK_TEST_NEST_MAP_ONE_KEY(
            "NestedKey0",RSK_TEST_NEST_MAP_ONE_KEY(
                "NestedNestedKey0",RSK_TEST_STR_VARIANT("NestedNestedValue0")
                )
            )
         }
                         })
    }};
    JsonDict source2_b(src_map2_b);
    QStringList expected2_b{"TestKey0","TestKey1","TestKey3"};
    QTest::newRow("Deeper nesting b") << source2_b << expected2_b;
}

void TestRedisStreamEntryFormatter::TestEntryKeys(){
    QFETCH(JsonDict, source);
    QFETCH(QStringList, expected_keys);
    QCOMPARE(RedisStreamEntryFormatter(source).entryKeys(), expected_keys);
}

void TestRedisStreamEntryFormatter::TestEventDataDict_data(){
    QTest::addColumn<JsonDict>("source");
    QTest::addColumn<JsonDict>("expected_dict");
    ///////// 0: Basic test
    QVariantMap src_map0{
    {"TopKey0", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")}})
     }};
    QVariantMap exp_map0{
         {"TestKey0", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")}};
    JsonDict source0(src_map0);
    JsonDict expected0(exp_map0);
    QTest::newRow("Basic test") << source0 << expected0;
    ///////// 1: 2 lvls nesting
    QVariantMap src_map1_a{
    {"TopKey0", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")}})
     }};
    QVariantMap src_map1_b{
    {"TopKey1", QVariant(QVariantMap{
         {"TestKey2", RSK_TEST_STR_VARIANT("Value2")},
         {"TestKey3", RSK_TEST_STR_VARIANT("Value3")}})
     }};
    JsonDict source1(QVariantMap{
                         {"TopTopKey0", QVariant(src_map1_a)},
                         {"TopTopKey1", QVariant(src_map1_b)}
                     });
    JsonDict expected1(src_map1_a); // should be same as nested map under first key
    QTest::newRow("2 top level keys (first should be used)") << source1 << expected1;
    ///////// 2: Deeper nesting
    QVariantMap src_map2_a{
    {"TopKey0", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_NEST_MAP_ONE_KEY(
            "NestedKey0",RSK_TEST_STR_VARIANT("Value0"))},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")},
         {"TestKey2", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey3", RSK_TEST_STR_VARIANT("Value1")}})
     }};
    auto exp_map2_a = QVariantMap{{"TestKey0", RSK_TEST_NEST_MAP_ONE_KEY(
                                      "NestedKey0",RSK_TEST_STR_VARIANT("Value0"))},
                                   {"TestKey1", RSK_TEST_STR_VARIANT("Value1")},
                                   {"TestKey2", RSK_TEST_STR_VARIANT("Value0")},
                                   {"TestKey3", RSK_TEST_STR_VARIANT("Value1")}};
    JsonDict source2(src_map2_a);
    JsonDict expected2(exp_map2_a);
    QTest::newRow("Deeper nesting") << source2 << expected2;
    //
    QVariantMap src_map2_b{
    {"TopKey0", QVariant(QVariantMap{
         {"TestKey0", RSK_TEST_NEST_MAP_ONE_KEY(
            "NestedKey0",RSK_TEST_STR_VARIANT("Value0"))},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey3", RSK_TEST_NEST_MAP_ONE_KEY(
            "NestedKey0",RSK_TEST_NEST_MAP_ONE_KEY(
                "NestedNestedKey0",RSK_TEST_STR_VARIANT("NestedNestedValue0")
                )
            )
         }
                         })
    }};
    QVariantMap exp_map2_b{
         {"TestKey0", RSK_TEST_NEST_MAP_ONE_KEY(
            "NestedKey0",RSK_TEST_STR_VARIANT("Value0"))},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value1")},
         {"TestKey1", RSK_TEST_STR_VARIANT("Value0")},
         {"TestKey3", RSK_TEST_NEST_MAP_ONE_KEY(
            "NestedKey0",RSK_TEST_NEST_MAP_ONE_KEY(
                "NestedNestedKey0",RSK_TEST_STR_VARIANT("NestedNestedValue0")
                )
            )
         }};
    JsonDict source2_b(src_map2_b);
    JsonDict expected2_b(exp_map2_b);
    QTest::newRow("Deeper nesting b") << source2_b << expected2_b;
}

void TestRedisStreamEntryFormatter::TestEventDataDict(){
    QFETCH(JsonDict, source);
    QFETCH(JsonDict, expected_dict);
    RSK_TEST_COMPARE_JSON_DICTS(RedisStreamEntryFormatter(source).eventDataDict(), expected_dict);
}
QTEST_GUILESS_MAIN(TestRedisStreamEntryFormatter);
