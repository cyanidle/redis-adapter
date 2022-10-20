#ifndef RSK_TEST_TIMEFORMATTER_H
#define RSK_TEST_TIMEFORMATTER_H


#include <QtTest>
#include <QString>

class TestTimeFormatter : public QObject
{
    Q_OBJECT
private slots:
    void TestToMsecsString();
    void TestDateTime();
};

#endif // RSK_TEST_TIMEFORMATTER_H
