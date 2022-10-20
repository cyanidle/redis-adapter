#include "tst_timeformatter.h"
#include "formatters/timeformatter.h"

#define MILLISECONDS_FORMAT "yyyy-MM-ddThh:mm:ss.zzz"
#define RSK_TEST_TIMESTAMP 1651574450000
#define RSK_TEST_EXPECTED_TIMESTRING "2022-05-03T10:40:50.000"


void TestTimeFormatter::TestToMsecsString(){
    TimeFormatter time(RSK_TEST_TIMESTAMP, "UTC+00:00");
    auto exp = QString(RSK_TEST_EXPECTED_TIMESTRING);
    QCOMPARE(time.toMSecsString(), exp);
}

void TestTimeFormatter::TestDateTime(){
    TimeFormatter time(RSK_TEST_TIMESTAMP);
    auto q_date_test = QDateTime::fromMSecsSinceEpoch(RSK_TEST_TIMESTAMP);
    QCOMPARE(time.dateTime(), q_date_test); // Проверяем весь объект
    //Потом можно бдует проверить отдельно секунды/часы и т.д.
}

QTEST_GUILESS_MAIN(TestTimeFormatter)
//#include "testtimeformatter.moc"
