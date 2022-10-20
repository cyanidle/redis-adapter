#include "tst_modbusformatter.h"
#include "rsktestmacros.h"
#include "formatters/modbusformatter.h"

//#define RSK_TEST_TIMESTAMP "2022-05-11T12:19:28.000"

void TestModbusFormatter::TestToModbusUnit_data(){
    QTest::addColumn<JsonDict>("source");
    QTest::addColumn<JsonDict>("expected");
    //////////////////////////////////////
    QVariantMap src_map0{
      {RSK_TEST_TIMESTRING, RSK_TEST_NEST_MAP_ONE_KEY(
          "station:16:command:network_rssi", QVariant(QString("3")))}
    };
    QVariantMap exp_map0{
        {"station",RSK_TEST_NEST_MAP_ONE_KEY(
                        "16",RSK_TEST_NEST_MAP_ONE_KEY(
                            "command",RSK_TEST_NEST_MAP_ONE_KEY(
                                "network_rssi",RSK_TEST_STR_VARIANT("3"))
                                )
                            )
                        }
    };
    JsonDict source0(src_map0);
    JsonDict expected0(exp_map0);
    QTest::newRow("Basic") << source0 << expected0;
    //////////////////////////////////////
    QVariantMap src_map1{
      {RSK_TEST_TIMESTRING, RSK_TEST_NEST_MAP_ONE_KEY("station:16:command:network_rssi:more_nesting", QVariant(QString("3")))}
    };
    QVariantMap exp_map1{
        {"station",RSK_TEST_NEST_MAP_ONE_KEY(
                        "16",RSK_TEST_NEST_MAP_ONE_KEY(
                            "command",RSK_TEST_NEST_MAP_ONE_KEY(
                                "network_rssi",RSK_TEST_NEST_MAP_ONE_KEY(
                                    "more_nesting",RSK_TEST_STR_VARIANT("3"))
                                    )
                                )
                            )
                        }
    };
    JsonDict source1(src_map1);
    JsonDict expected1(exp_map1);
    QTest::newRow("Changed") << source1 << expected1;
    //////////////////////////////////////
    QVariantMap src_map2{
      {"NotATimestamp", RSK_TEST_NEST_MAP_ONE_KEY("station:16:command:network_rssi", QVariant(QString("3")))}
    };
    QVariantMap exp_map2{}; // Should be empty
    JsonDict source2(src_map2);
    JsonDict expected2(exp_map2);
    QTest::newRow("Wrong Timestamp") << source2 << expected2;
    /////////////////////////////////////
}

void TestModbusFormatter::TestToModbusUnit(){
    QFETCH(JsonDict, source);
    QFETCH(JsonDict, expected);
    auto actual = ModbusFormatter(source).toModbusUnit();
    RSK_TEST_COMPARE_JSON_DICTS(actual, expected);
}

QTEST_GUILESS_MAIN(TestModbusFormatter);
