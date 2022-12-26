#include "tst_archivequeryformatter.h"
#include "include/sqlarchivefields.h"

void TestArchiveQueryFormatter::TestToWriteRecordsList_data()
{
    QTest::addColumn<JsonDict>("sourceRedisStreamEntry");
    QTest::addColumn<JsonDict>("sourceKeyVaultEntries");
    QTest::addColumn<JsonList>("expectedList");
    ////// Basic test
    QVariantMap src_redis_map0{
    {RSK_TEST_TIMESTRING, QVariant(QVariantMap{
         {"station:16:pump", RSK_TEST_STR_VARIANT("run")},
         {"station:16:sensors", RSK_TEST_STR_VARIANT("stop")},
         {"station:16:commands", RSK_TEST_STR_VARIANT("continue")}})
     }};
    QVariantMap src_keyvault_map0a{
        {"station:16:pump", QVariant(QVariantMap{
                                        {SQL_ARCHIVE_FIELD_SOURCE_TYPE, RSK_TEST_STR_VARIANT("01")},
                                        {SQL_ARCHIVE_FIELD_SOURCE, RSK_TEST_STR_VARIANT("11")},
                                        {SQL_ARCHIVE_FIELD_PARAM, RSK_TEST_STR_VARIANT("21")},
                                        })},
        {"station:16:sensors", QVariant(QVariantMap{
                                        {SQL_ARCHIVE_FIELD_SOURCE_TYPE, RSK_TEST_STR_VARIANT("02")},
                                        {SQL_ARCHIVE_FIELD_SOURCE, RSK_TEST_STR_VARIANT("12")},
                                        {SQL_ARCHIVE_FIELD_PARAM, RSK_TEST_STR_VARIANT("22")},
                                        })},
        {"station:16:commands", QVariant(QVariantMap{
                                        {SQL_ARCHIVE_FIELD_SOURCE_TYPE, RSK_TEST_STR_VARIANT("03")},
                                        {SQL_ARCHIVE_FIELD_SOURCE, RSK_TEST_STR_VARIANT("13")},
                                        {SQL_ARCHIVE_FIELD_PARAM, RSK_TEST_STR_VARIANT("23")},
                                        })}
    };
    QVariantMap init_exp_map0a{
            {SQL_ARCHIVE_FIELD_SOURCE_TYPE, RSK_TEST_STR_VARIANT("01")},
            {SQL_ARCHIVE_FIELD_SOURCE, RSK_TEST_STR_VARIANT("11")},
            {SQL_ARCHIVE_FIELD_PARAM, RSK_TEST_STR_VARIANT("21")},
            {SQL_ARCHIVE_FIELD_EVENT_TIME, QVariant(QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT))},
            {SQL_ARCHIVE_FIELD_SERVER_TIME,QVariant(QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT))},
            {SQL_ARCHIVE_FIELD_VALUE, RSK_TEST_STR_VARIANT("run")}
    };
    QVariantMap init_exp_map0b{
            {SQL_ARCHIVE_FIELD_SOURCE_TYPE, RSK_TEST_STR_VARIANT("02")},
            {SQL_ARCHIVE_FIELD_SOURCE, RSK_TEST_STR_VARIANT("12")},
            {SQL_ARCHIVE_FIELD_PARAM, RSK_TEST_STR_VARIANT("22")},
            {SQL_ARCHIVE_FIELD_EVENT_TIME, QVariant(QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT))},
            {SQL_ARCHIVE_FIELD_SERVER_TIME,QVariant(QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT))},
            {SQL_ARCHIVE_FIELD_VALUE, RSK_TEST_STR_VARIANT("stop")}
    };
    QVariantMap init_exp_map0c{
            {SQL_ARCHIVE_FIELD_SOURCE_TYPE, RSK_TEST_STR_VARIANT("03")},
            {SQL_ARCHIVE_FIELD_SOURCE, RSK_TEST_STR_VARIANT("13")},
            {SQL_ARCHIVE_FIELD_PARAM, RSK_TEST_STR_VARIANT("23")},
            {SQL_ARCHIVE_FIELD_EVENT_TIME, QVariant(QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT))},
            {SQL_ARCHIVE_FIELD_SERVER_TIME,QVariant(QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT))},
            {SQL_ARCHIVE_FIELD_VALUE, RSK_TEST_STR_VARIANT("continue")}
    };
   JsonDict sourceRedis0(src_redis_map0);
   JsonDict sourceKeyVault0(src_keyvault_map0a);
    JsonList expectedList0(QVariantList{
                               QVariant(init_exp_map0a),
                               QVariant(init_exp_map0b),
                               QVariant(init_exp_map0c)
                           });
    QTest::newRow("Basic test") << sourceRedis0 << sourceKeyVault0 << expectedList0;
}

void TestArchiveQueryFormatter::TestToWriteRecordsList()
{
    QFETCH(JsonDict, sourceRedisStreamEntry);
    QFETCH(JsonDict, sourceKeyVaultEntries);
    QFETCH(JsonList, expectedList);
    ArchiveQueryFormatter formatter(sourceRedisStreamEntry);
    auto actual = formatter.toWriteRecordsList(sourceKeyVaultEntries);
    QVariantMap currentMap;
    for (int index = 0; index < actual.data().size(); index++){ // This part swaps current server time with fake one
        currentMap = actual.data().takeAt(index).toMap();                 // in dicts of the 'actual' list
        currentMap.remove(SQL_ARCHIVE_FIELD_EVENT_TIME);
        currentMap.insert(SQL_ARCHIVE_FIELD_EVENT_TIME,
                        QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT));
        currentMap.remove(SQL_ARCHIVE_FIELD_SERVER_TIME);
        currentMap.insert(SQL_ARCHIVE_FIELD_SERVER_TIME,
                        QDateTime::fromString(RSK_TEST_TIMESTRING, MILLISECONDS_FORMAT));
        actual.data().insert(index,QVariant(currentMap));
    }
    RskTestLib::compareJsonLists(actual,expectedList); // the comparison itself
}

QTEST_GUILESS_MAIN(TestArchiveQueryFormatter)


