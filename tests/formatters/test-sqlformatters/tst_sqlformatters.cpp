#include "tst_sqlformatters.h"
#include "rsktestmacros.h"
// QTest::addColumn<JsonDict>("source");
    // QTest::addColumn<QDateTime>("expected");
    // ////// Basic test
    // QTest::newRow("Basic test") << source1 << expected1;
    // QTest::newRow("Basic test") << source1 << expected1_b;
using namespace MySql;
void TestSqlFormatters::initTestCase()
{
}

// QueryFields
void TestSqlFormatters::TestToSqlRecord_data()
{
    QTest::addColumn<JsonDict>("source_dict");
    QTest::addColumn<JsonList>("source_list");
    QTest::addColumn<QueryRecord>("expected");
    ////// Basic test
    JsonDict source_dict0(QVariantMap{
        {"Field0",QVariant()},
        {"Field1",QVariant()},
        {"Field2",QVariant()},
        {"Field3",QVariant()}
    });
    JsonList source_list0(QVariantList{
        RSK_TEST_STR_VARIANT("Field0"), RSK_TEST_STR_VARIANT("Field1"), RSK_TEST_STR_VARIANT("Field2"), RSK_TEST_STR_VARIANT("Field3")
    });
    QueryRecord expected0{
        QueryField("Field0",QVariant()), QueryField("Field1",QVariant()),QueryField("Field2",QVariant()),QueryField("Field3",QVariant())
    };
    QTest::newRow("Basic test") << source_dict0 << source_list0 << expected0;
}

void TestSqlFormatters::TestToSqlRecord()
{
    QFETCH(JsonDict, source_dict);
    QFETCH(JsonList, source_list);
    QFETCH(QueryRecord,expected);
    RSK_TEST_COMPARE_QUERY_RECORDS(SqlQueryFieldsFormatter(source_dict).toSqlRecord(), expected);
    RSK_TEST_COMPARE_QUERY_RECORDS(SqlQueryFieldsFormatter(source_list).toSqlRecord(), expected);
    //QFAIL("(Тест проходит 13.05.22) Стоит подумать над оператором сравнения для объектов QueryField!");
}

void TestSqlFormatters::TestToSqlRecordList_data() // Almost same as last test
{
    QTest::addColumn<JsonDict>("source_dict");
    QTest::addColumn<JsonList>("source_list");
    QTest::addColumn<QueryRecordList>("expected");
    ////// Basic test
    JsonDict source_dict0(QVariantMap{
        {"Field0",QVariant()},
        {"Field1",QVariant()},
        {"Field2",QVariant()},
        {"Field3",QVariant()}
    });
    JsonList source_list0(QVariantList{
        RSK_TEST_STR_VARIANT("Field0"), RSK_TEST_STR_VARIANT("Field1"), RSK_TEST_STR_VARIANT("Field2"), RSK_TEST_STR_VARIANT("Field3")
    });
    QueryRecordList expected0{ QueryRecord{
        QueryField("Field0",QVariant()), QueryField("Field1",QVariant()),QueryField("Field2",QVariant()),QueryField("Field3",QVariant())
    }};
    QTest::newRow("Basic test") << source_dict0 << source_list0 << expected0;
}

void TestSqlFormatters::TestToSqlRecordList()
{
    QFETCH(JsonDict, source_dict);
    QFETCH(JsonList, source_list);
    QFETCH(QueryRecordList,expected);
    RSK_TEST_COMPARE_QUERY_RECORDS(SqlQueryFieldsFormatter(source_dict).toSqlRecord(), expected[0]);
    RSK_TEST_COMPARE_QUERY_RECORDS(SqlQueryFieldsFormatter(source_list).toSqlRecord(), expected[0]);
}

// Query
void TestSqlFormatters::TestToRegExpFilter_data()
{
    QTest::addColumn<QString>("fieldName");
    QTest::addColumn<QStringList>("keys");
    QTest::addColumn<QString>("expected");
    ////// Basic test
    QString fieldName0("MY_DB");
    QStringList keys0{"KEY0","KEY1","KEY2","KEY3"};
    QString expected0("WHERE MY_DB REGEXP 'KEY0|KEY1|KEY2|KEY3'");
    QTest::newRow("Basic test") << fieldName0 << keys0 << expected0;
    ////// Complex test
    QString fieldName1("MY_BETTER_DB");
    QStringList keys1{"KEY0:station:16:complex:key","KEY1","KEY2","KEY3:station:16:commands"};
    QString expected1(
        "WHERE MY_BETTER_DB REGEXP 'KEY0:station:16:complex:key|KEY1|KEY2|KEY3:station:16:commands'");
    QTest::newRow("Complex keys") << fieldName1 << keys1 << expected1;
}

void TestSqlFormatters::TestToRegExpFilter()
{
    QFETCH(QString,fieldName);
    QFETCH(QStringList,keys);
    QFETCH(QString,expected);
    QCOMPARE(SqlQueryFormatter().toRegExpFilter(fieldName,keys), expected);
}

// Result
void TestSqlFormatters::TestToJsonRecord_data()
{
    QTest::addColumn<QueryRecordList>("source");
    QTest::addColumn<JsonList>("expected");
    ////// Basic test
    QueryRecordList source0{
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field0"))},
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field1"))},
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field2"))},
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field3"))},
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field4"))}
    };
    QVariantList exp_list0{
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field0")),
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field1")),
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field2")),
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field3")),
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field4"))
    };
    JsonList expected0(exp_list0);
    QTest::newRow("Basic test") << source0 << expected0;
    ////// Nesting
    QueryRecordList source1{
        QueryRecord{
                    QueryField("MyDb0", RSK_TEST_STR_VARIANT("field0")),
                    QueryField("MyDb1", RSK_TEST_STR_VARIANT("field1")),
                    QueryField("MyDb2", RSK_TEST_STR_VARIANT("field2"))
        },
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field1"))},
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field2"))},
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field3"))},
        QueryRecord{QueryField("MyDb", RSK_TEST_STR_VARIANT("field4"))}
    };
    QVariantList exp_list1{
        QVariant(QVariantMap{
            {"MyDb0", RSK_TEST_STR_VARIANT("field0")},
            {"MyDb1", RSK_TEST_STR_VARIANT("field1")},
            {"MyDb2", RSK_TEST_STR_VARIANT("field2")}
        }),
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field1")),
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field2")),
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field3")),
        RSK_TEST_NEST_MAP_ONE_KEY("MyDb",RSK_TEST_STR_VARIANT("field4"))
    };
    JsonList expected1(exp_list1);
    QTest::newRow("Nesting") << source1 << expected1;
    //
    source1.append(QueryRecord{
                       QueryField("MyDb0", RSK_TEST_STR_VARIANT("field0")),
                       QueryField("MyDb1", RSK_TEST_STR_VARIANT("field1")),
                       QueryField("MyDb2", RSK_TEST_STR_VARIANT("field2"))
           });
    expected1.data().append(QVariant(QVariantMap{
                                         {"MyDb0", RSK_TEST_STR_VARIANT("field0")},
                                         {"MyDb1", RSK_TEST_STR_VARIANT("field1")},
                                         {"MyDb2", RSK_TEST_STR_VARIANT("field2")}
                                     }));
    QTest::newRow("Nesting (appending)") << source1 << expected1;
}

void TestSqlFormatters::TestToJsonRecord()
{
    QFETCH(QueryRecordList, source);
    QFETCH(JsonList, expected);
    auto actual = SqlResultFormatter(source).toJsonList();
    QCOMPARE(actual.data(), expected.data());
}

QTEST_GUILESS_MAIN(TestSqlFormatters)
