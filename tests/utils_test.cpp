#include "utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryFile>
#include <QTextStream>

#include <QtTest>

class TestUtils : public QObject
{
    Q_OBJECT

private slots:
    void test_readTextFile();

    void test_sysexTextFile_data();
    void test_sysexTextFile();

    void test_sysexTextFileRejectsGarbage_data();
    void test_sysexTextFileRejectsGarbage();

    void test_sysexTextFileRejectsTrailingText();
    void test_writeProgramFileReportsErrors();
};

void TestUtils::test_readTextFile()
{
    QTemporaryFile f;
    if (!f.open()) {
        QFAIL("Cannot open temporary file");
    }

    const QString p = f.fileName();

    QCOMPARE(readTextFile(p), QString());

    QTextStream s(&f);
    s << 0 << " " << 127 << " " << 256;
    f.close();

    QCOMPARE(readTextFile(f.fileName()), QString("0 127 256"));
    QVERIFY_THROWS_EXCEPTION(std::runtime_error, readTextFile("This path does not exist"));
}

void TestUtils::test_sysexTextFile_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<int>("readCount");
    QTest::addColumn<char>("readBegin");
    QTest::addColumn<char>("readEnd");
    QTest::addColumn<QString>("error");

    QTest::newRow("Short") << QByteArrayLiteral("\x70\x60\x7f") << 0 << char(0) << char(0)
                           << "ends too early: 3 values instead of 66";

    QByteArray long_buf;
    for (char c = 0; c < 100; ++c) {
        long_buf.append(c);
    }
    // More than one program's worth of values: rejected rather than cut.
    QTest::newRow("Long") << long_buf << 0 << char(0) << char(0) << "has data after the 66 values";

    QByteArray exact;
    for (char c = 0; c < 66; ++c) {
        exact.append(c);
    }
    QTest::newRow("Exactly 66") << exact << 66 << char(0) << char(65) << QString();
}

void TestUtils::test_sysexTextFile()
{
    QTemporaryFile f;
    if (!f.open()) {
        QFAIL("Cannot open temporary file");
    }
    const QString p = f.fileName();
    f.close();

    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(QByteArray, bytes);
    QFETCH(int, readCount);
    QFETCH(char, readBegin);
    QFETCH(char, readEnd);
    QFETCH(QString, error);
    // NOLINTEND(misc-const-correctness)

    QVERIFY(writeProgramFile(bytes, p).has_value());

    const auto read = fromSysexTextFile(p);
    if (!error.isNull()) {
        QVERIFY(!read.has_value());
        QVERIFY2(read.error().contains(error), qPrintable(read.error()));
        return;
    }
    QVERIFY(read.has_value());
    QCOMPARE(read->size(), readCount);
    QCOMPARE(read->at(0), readBegin);
    QCOMPARE(read->at(readCount - 1), readEnd);
}

void TestUtils::test_sysexTextFileRejectsGarbage_data()
{
    QTest::addColumn<QByteArray>("content");

    // 65 valid values, then a bad one.
    QByteArray valid;
    for (int i = 0; i < 65; ++i) {
        valid += "1 ";
    }
    QTest::newRow("Not a number") << valid + "abc";
    QTest::newRow("Above a byte") << valid + "300";
    QTest::newRow("Negative") << valid + "-1";
}

void TestUtils::test_sysexTextFileRejectsGarbage()
{
    // NOLINTNEXTLINE(misc-const-correctness): QFETCH declares the variable
    QFETCH(QByteArray, content);

    QTemporaryFile f;
    QVERIFY(f.open());
    f.write(content);
    f.close();

    const auto read = fromSysexTextFile(f.fileName());
    QVERIFY(!read.has_value());
    QVERIFY2(read.error().contains("invalid value at position 65"), qPrintable(read.error()));
}

void TestUtils::test_sysexTextFileRejectsTrailingText()
{
    // 66 valid values, then text: there are not more *values*, so the error
    // must not say so.
    QByteArray content;
    for (int i = 0; i < 66; ++i) {
        content += "1 ";
    }
    content += "abc";

    QTemporaryFile f;
    QVERIFY(f.open());
    f.write(content);
    f.close();

    const auto read = fromSysexTextFile(f.fileName());
    QVERIFY(!read.has_value());
    QVERIFY2(read.error().contains("has data after the 66 values"), qPrintable(read.error()));
}

void TestUtils::test_writeProgramFileReportsErrors()
{
    const auto written = writeProgramFile(QByteArray(66, 0), "/this/folder/does/not/exist/p.lpd8");
    QVERIFY(!written.has_value());
    QVERIFY2(written.error().startsWith("Cannot write"), qPrintable(written.error()));
}

QTEST_APPLESS_MAIN(TestUtils)

#include "utils_test.moc"
