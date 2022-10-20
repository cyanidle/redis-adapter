#ifndef RSK_TEST_KEYVAULTRESULTFORMATTER_H
#define RSK_TEST_KEYVAULTRESULTFORMATTER_H
#include <QtTest>
#include "formatters/keyvaultresultformatter.h"
#include "include/sqlkeyvaultfields.h"
#include "rsktestlib.h"

class TestKeyVaultResultFormatter : public QObject
{
    Q_OBJECT
private slots:
    void TestToJsonEntry_data();
    void TestToJsonEntry();
    void TestIsValid_data();
    void TestIsValid();
};

#endif // RSK_TEST_KEYVAULTRESULTFORMATTER_H
