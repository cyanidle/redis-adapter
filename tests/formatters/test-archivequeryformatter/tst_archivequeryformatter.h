#include <QtTest>
#include "formatters/archivequeryformatter.h"
#include "formatters/redisstreamentryformatter.h"
#include "formatters/keyvaultresultformatter.h"
#include "formatters/timeformatter.h"
#include "rsktestlib.h"
class TestArchiveQueryFormatter : public QObject
{
    Q_OBJECT
private slots:
    void TestToWriteRecordsList_data();
    void TestToWriteRecordsList();
};
