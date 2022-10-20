#ifndef RSK_TEST_MODBUSFORMATTER_H
#define RSK_TEST_MODBUSFORMATTER_H
#include <QtTest>
#include "formatters/modbusformatter.h"

class TestModbusFormatter : public QObject
{
    Q_OBJECT
private slots:
    void TestToModbusUnit_data();
    void TestToModbusUnit();
};

#endif // RSK_TEST_TIMEFORMATTER_H
