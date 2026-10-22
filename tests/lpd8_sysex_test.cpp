#include "lpd8_sysex.h"

#include "test_sysex.h"

using namespace sysex;

#include <QtTest>

class TestLpd8Sysex : public QObject
{
    Q_OBJECT

private slots:
    void test_programRequestSize();
    void test_padsCount();
    void test_knobsCount();
    void test_makeSetProgramRequest();
    void test_type();
    void test_addProgramHeader();
    void test_addFooter();
    void test_getProgram();
    void test_channel();
};

void TestLpd8Sysex::test_programRequestSize()
{
    QCOMPARE(programRequestSize(), 66);
}

void TestLpd8Sysex::test_padsCount()
{
    QCOMPARE(padsCount(), 8);
}

void TestLpd8Sysex::test_knobsCount()
{
    QCOMPARE(knobsCount(), 8);
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
    sysex[4] = '\x63';
    sysex[5] = '\x00';
    sysex[6] = '\x3a';
    sysex[sysex.size()-1] = '\xf7';
    QCOMPARE(type(sysex), Type::TypeProgram);
}

void TestLpd8Sysex::test_addProgramHeader()
{
    QByteArray sysex;
    addProgramHeader(sysex, 3);
    QCOMPARE(sysex, QByteArrayLiteral("\xf0\x47\x7f\x75\x63\x00\x01\x03"));
}

void TestLpd8Sysex::test_addFooter()
{
    QByteArray sysex;
    addFooter(sysex);
    QCOMPARE(sysex.size(), 1);
    QCOMPARE(sysex.at(0), '\xf7');
}

void TestLpd8Sysex::test_getProgram()
{
    QCOMPARE(getProgram(3), QByteArrayLiteral("\xf0\x47\x7f\x75\x63\x00\x01\x03\xf7"));
}

void TestLpd8Sysex::test_channel()
{
    QCOMPARE(channel(default_sysex), '\x09');
}

QTEST_APPLESS_MAIN(TestLpd8Sysex)

#include "lpd8_sysex_test.moc"
