#include "midivaluedelegate.h"

#include <QtTest>

class Exposed : public LutSpinBox {
public:
    using LutSpinBox::textFromValue;
    using LutSpinBox::valueFromText;
};

class TestMidiValueDelegate : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_textFromValue_data();
    void test_textFromValue();

private:
    Exposed* sb;
};

void TestMidiValueDelegate::initTestCase()
{
    sb = new Exposed();
    sb->setLut(QStringList() << "Zero" << "One");
}

void TestMidiValueDelegate::cleanupTestCase()
{
    sb->deleteLater();
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

    QCOMPARE(sb->valueFromText(text), value);
    if (sb->valueFromText(text) >= 0) {
        QCOMPARE(sb->textFromValue(value), text);
    }
}

QTEST_MAIN(TestMidiValueDelegate)

#include "tst_MidiValueDelegate.moc"
