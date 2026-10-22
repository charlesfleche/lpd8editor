#include "deviceprofile.h"
#include "program.h"

#include <QtTest>

using namespace lpd8;

class TestProgram : public QObject
{
    Q_OBJECT

private slots:
    void test_equality();
    void test_isValid_data();
    void test_isValid();
    void test_lpd8DefaultProgram();
};

void TestProgram::test_equality()
{
    const Program a = lpd8::profile().defaultProgram;
    Program b = a;
    QCOMPARE(a, b);

    b.knobs[7].high = 100;
    QVERIFY(a != b);

    b = a;
    b.channel = 3;
    QVERIFY(a != b);
}

void TestProgram::test_isValid_data()
{
    QTest::addColumn<Program>("program");
    QTest::addColumn<bool>("valid");

    const Program valid = lpd8::profile().defaultProgram;
    QTest::newRow("Default") << valid << true;

    Program p = valid;
    p.channel = 16;
    QTest::newRow("Channel 17") << p << false;

    p = valid;
    p.pads[3].note = 128;
    QTest::newRow("Note above 127") << p << false;

    p = valid;
    p.knobs[0].high = 200;
    QTest::newRow("Knob range above 127") << p << false;

    p = valid;
    p.pads[7].controlChange = 127;
    p.knobs[7].low = 127;
    QTest::newRow("127 is allowed") << p << true;
}

void TestProgram::test_isValid()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(Program, program);
    QFETCH(bool, valid);
    // NOLINTEND(misc-const-correctness)

    QCOMPARE(program.isValid(), valid);
}

void TestProgram::test_lpd8DefaultProgram()
{
    const DeviceProfile &profile = lpd8::profile();
    QCOMPARE(profile.programSlots, 4);

    // The values of the SQL trigger that created programs until now
    // (create_trigger_add_program.sql).
    const Program &p = profile.defaultProgram;
    QCOMPARE(p.channel, quint8(9));
    QCOMPARE(p.pads[0],
             (Pad{.note = 36, .programChange = 1, .controlChange = 48, .toggle = false}));
    QCOMPARE(p.pads[7],
             (Pad{.note = 43, .programChange = 8, .controlChange = 19, .toggle = false}));
    QCOMPARE(p.knobs[0], (Knob{.controlChange = 16, .low = 0, .high = 127}));
    QCOMPARE(p.knobs[7], (Knob{.controlChange = 51, .low = 0, .high = 127}));
    QVERIFY(p.isValid());
}

QTEST_APPLESS_MAIN(TestProgram)

#include "program_test.moc"
