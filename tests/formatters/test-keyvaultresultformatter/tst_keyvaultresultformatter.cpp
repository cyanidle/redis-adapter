#include "tst_keyvaultresultformatter.h"

void TestKeyVaultResultFormatter::TestToJsonEntry_data(){
    QTest::addColumn<JsonList>("source");
    QTest::addColumn<JsonDict>("expected");
    QVariantMap entry0a{
        {SQL_KEYVAULT_FIELD_REDIS_KEY, QVariant("my_redis_key0")},
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type")},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant("source")},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant("param")}
    };
    QVariantMap entry0b{
        {SQL_KEYVAULT_FIELD_REDIS_KEY, QVariant("my_redis_key1")},
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type")},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant("source")},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant("param")}
    };
    QVariantMap entry0c{
        {SQL_KEYVAULT_FIELD_REDIS_KEY, QVariant("my_redis_key2")},
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type")},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant("source")},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant("param")}
    };
    QVariantMap exp_map0{{SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type")},
    {SQL_KEYVAULT_FIELD_SOURCE, QVariant("source")},
    {SQL_KEYVAULT_FIELD_PARAM, QVariant("param")}};
    JsonList source0(QVariantList{QVariant(entry0a),QVariant(entry0b),QVariant(entry0c)});
    JsonDict expected0(QVariantMap{
                           {"my_redis_key0",QVariant(exp_map0)},
                           {"my_redis_key1",QVariant(exp_map0)},
                           {"my_redis_key2",QVariant(exp_map0)}
                       });
    QTest::newRow("Basic test") << source0 << expected0;
    // More nesting and different values
    QVariantMap entry1a{
        {SQL_KEYVAULT_FIELD_REDIS_KEY, QVariant("my_redis_key0")},
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type0")},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant("source0")},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant("param0")}
    };
    QVariantMap entry1b{
        {SQL_KEYVAULT_FIELD_REDIS_KEY, QVariant("my_redis_key1")},
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type")},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant("source")},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant("param")}
    };
    QVariantMap entry1c{
        {SQL_KEYVAULT_FIELD_REDIS_KEY, QVariant("my_redis_key2")},
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type")},
        {SQL_KEYVAULT_FIELD_SOURCE, RSK_TEST_NEST_MAP_ONE_KEY("top_source",QVariant("source"))},
        {SQL_KEYVAULT_FIELD_PARAM, RSK_TEST_NEST_MAP_ONE_KEY("top_param",QVariant("param"))}
    };
    JsonList source1(QVariantList{QVariant(entry1a),QVariant(entry1b),QVariant(entry1c)});
    JsonDict expected1(QVariantMap{
                           {"my_redis_key0",QVariant(
                            QVariantMap{{SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type0")},
                            {SQL_KEYVAULT_FIELD_SOURCE, QVariant("source0")},
                            {SQL_KEYVAULT_FIELD_PARAM, QVariant("param0")}}
                            )},
                           {"my_redis_key1",QVariant(
                            QVariantMap{{SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type")},
                            {SQL_KEYVAULT_FIELD_SOURCE, QVariant("source")},
                            {SQL_KEYVAULT_FIELD_PARAM, QVariant("param")}}
                            )},
                           {"my_redis_key2",QVariant(
                            QVariantMap{{SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant("type")},
                            {SQL_KEYVAULT_FIELD_SOURCE, RSK_TEST_NEST_MAP_ONE_KEY("top_source",QVariant("source"))},
                            {SQL_KEYVAULT_FIELD_PARAM, RSK_TEST_NEST_MAP_ONE_KEY("top_param",QVariant("param"))}}
                            )}
                       });
    QTest::newRow("Nesting and different field specifiers") << source1 << expected1;
}

void TestKeyVaultResultFormatter::TestToJsonEntry(){
    QFETCH(JsonList, source);
    QFETCH(JsonDict, expected);
    auto actual = KeyVaultResultFormatter(source).toJsonEntries();
    RSK_TEST_COMPARE_JSON_DICTS(actual,expected);
}

void TestKeyVaultResultFormatter::TestIsValid_data(){
    QTest::addColumn<JsonDict>("source");
    QTest::addColumn<bool>("expected");
    JsonDict source0(QVariantMap{
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant(1)},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant(2)},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant(3)}
    });
    auto expected0 = true;
    QTest::newRow("Basic test") << source0 << expected0;
    // Invalids
    expected0 = false;
    source0.data().take(SQL_KEYVAULT_FIELD_SOURCE_TYPE);
    QTest::newRow("Missing key") << source0 << expected0;
    //
    JsonDict source1(QVariantMap{
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant(1)},
        {SQL_KEYVAULT_FIELD_SOURCE, RSK_TEST_NEST_MAP_ONE_KEY("Source",QVariant(2))},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant(3)}
    });
    auto expected1 = false;
    QTest::newRow("Nested value") << source1 << expected1;
    //
    JsonDict source2a(QVariantMap{
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant(quint32())},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant(2)},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant(3)}
    });
    auto expected2a = false;
    QTest::newRow("Empty key") << source2a << expected2a;
    //
    JsonDict source2b(QVariantMap{
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant()},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant(2)},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant(3)}
    });
    auto expected2b = false;
    QTest::newRow("Default initialised key") << source2b << expected2b;
    //
    JsonDict source2c(QVariantMap{
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, RSK_TEST_STR_VARIANT("1")},
        {SQL_KEYVAULT_FIELD_SOURCE, RSK_TEST_STR_VARIANT("2")},
        {SQL_KEYVAULT_FIELD_PARAM, RSK_TEST_STR_VARIANT("3")}
    });
    auto expected2c = true;
    QTest::newRow("String keys") << source2c << expected2c;
    //
    JsonDict source3(QVariantMap{
        {SQL_KEYVAULT_FIELD_REDIS_KEY, QVariant(11)},
        {SQL_KEYVAULT_FIELD_SOURCE_TYPE, QVariant(2)},
        {SQL_KEYVAULT_FIELD_SOURCE, QVariant(3)},
        {SQL_KEYVAULT_FIELD_PARAM, QVariant(4)}
    });
    auto expected3 = false;
    QTest::newRow("Extra key") << source3 << expected3;
}

void TestKeyVaultResultFormatter::TestIsValid(){
    QFETCH(JsonDict, source);
    QFETCH(bool, expected);
    auto actual = KeyVaultResultFormatter{}.isValid(source);
    QCOMPARE(actual, expected);
}

QTEST_GUILESS_MAIN(TestKeyVaultResultFormatter);
