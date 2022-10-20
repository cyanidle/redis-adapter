#ifndef TST_MODBUSCONNECTOR
#define TST_MODBUSCONNECTOR
#include <QtTest>
#include "connectors/modbusconnector.h"
#include "settings/reader.h"
#include "rsktestlib.h"
#include <QSignalSpy>

class tst_ModbusConnector : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void tst_Instances();
    void tst_Settings();
private:
    RskTestLib m_tester;
};

#endif //TST_MODBUSCONNECTOR
