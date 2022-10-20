#include "tst_modbusconnector.h"

void tst_ModbusConnector::initTestCase(){
    qDebug() << "Changing configs directory to ../testfiles/original";
    m_tester.getReader()->setFilesDirectory("../testfiles/original");
}

void tst_ModbusConnector::tst_Instances()
{
    auto defaultConnectionSettings = m_tester.getReader()->getModbusConnectionSettings();
    auto defaultDeviceRegisters = m_tester.getReader()->getRegisters();

    ModbusConnector::init(defaultConnectionSettings,defaultDeviceRegisters);
    auto inst1 = ModbusConnector::instance();
    ModbusConnector::init(defaultConnectionSettings,defaultDeviceRegisters);
    auto inst2 = ModbusConnector::instance();
    QCOMPARE(inst1, inst2);
}

void tst_ModbusConnector::tst_Settings()
{
    auto defaultConnectionSettings = m_tester.getReader()->getModbusConnectionSettings();
    auto defaultDeviceRegisters = m_tester.getReader()->getRegisters();
    ModbusConnector::init(defaultConnectionSettings,defaultDeviceRegisters);
    auto inst = ModbusConnector::instance();
    ModbusConnector::init(defaultConnectionSettings,defaultDeviceRegisters);
    auto actual = inst->settings();
    auto expected = m_tester.getDefaultModbusConSettings();
    QCOMPARE(actual,expected);
}


QTEST_GUILESS_MAIN(tst_ModbusConnector)
