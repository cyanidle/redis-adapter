#ifndef RSK_TEST_SQLFORMATTERS_H
#define RSK_TEST_SQLFORMATTERS_H
#include <QtTest>
#include "formatters/sqlqueryfieldsformatter.h"
#include "formatters/sqlqueryformatter.h"
#include "formatters/sqlresultformatter.h"

class TestSqlFormatters : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    // QueryFields
    void TestToSqlRecord_data();
    void TestToSqlRecord();
    void TestToSqlRecordList_data();
    void TestToSqlRecordList();
    // Query
    void TestToRegExpFilter_data();
    void TestToRegExpFilter();
    // Result
    void TestToJsonRecord_data();
    void TestToJsonRecord();
};

#endif // RSK_TEST_SQLFORMATTERS_H
