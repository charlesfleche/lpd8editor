#include "lpd8_sysex.h"

using namespace sysex;

#include <QtTest>

class TestLpd8Sysex : public QObject
{
    Q_OBJECT

private slots:
    void test_programRequestSize();
    void test_makeSetProgramRequest();
    void test_type();
    void test_getProgram();
};

void TestLpd8Sysex::test_programRequestSize()
{
    QCOMPARE(programRequestSize(), 66);
}

void TestLpd8Sysex::test_makeSetProgramRequest()
{
    QByteArray sysex(programRequestSize(), 0);
    const int id = 3;
    makeSetProgramRequest(sysex, id);
    QCOMPARE(sysex.at(4), '\x61');
    QCOMPARE(sysex.at(5), '\x00');
    QCOMPARE(sysex.at(6), '\x3a');
    QCOMPARE(sysex.at(7), static_cast<char>(id));
}

void TestLpd8Sysex::test_type()
{
    QByteArray sysex;
    QCOMPARE(type(sysex), Type::TypeInvalid);

    sysex = QByteArray(programRequestSize(), 0);
    QCOMPARE(type(sysex), Type::TypeInvalid);
    sysex[0] = '\xf0';
    sysex[1] = '\x47'; // Akai
    sysex[2] = '\x7f'; // LPD8
    sysex[3] = '\x75';
    sysex[4] = '\x63'; // program dump
    sysex[5] = '\x00';
    sysex[6] = '\x3a';
    sysex[sysex.size() - 1] = '\xf7';
    QCOMPARE(type(sysex), Type::TypeProgram);

    // Same message from another manufacturer's device
    sysex[1] = '\x41';
    QCOMPARE(type(sysex), Type::TypeInvalid);
}

void TestLpd8Sysex::test_getProgram()
{
    QCOMPARE(getProgram(3), QByteArrayLiteral("\xf0\x47\x7f\x75\x63\x00\x01\x03\xf7"));
}

QTEST_APPLESS_MAIN(TestLpd8Sysex)

#include "lpd8_sysex_test.moc"
