#include "midivaluedelegate.h"

#include <QtTest>

class TestMidiValueDelegate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void test_textFromValue_data();
    void test_textFromValue();

private:
    QStringList lut;
};

void TestMidiValueDelegate::initTestCase()
{
    lut << "Zero" << "One";
}

void TestMidiValueDelegate::test_textFromValue_data()
{
    QTest::addColumn<int>("value");
    QTest::addColumn<QString>("text");

    QTest::newRow("Invalid text") << -1 << "Invalid";
    QTest::newRow("Zero") << 0 << "Zero";
    QTest::newRow("One") << 1 << "One";
}

void TestMidiValueDelegate::test_textFromValue()
{
    QFETCH(int, value);
    QFETCH(QString, text);

    QCOMPARE(lutValueFromText(lut, text), value);
    if (lutValueFromText(lut, text) >= 0) {
        QCOMPARE(lutTextFromValue(lut, value), text);
    }
}

QTEST_APPLESS_MAIN(TestMidiValueDelegate)

#include "tst_MidiValueDelegate.moc"
