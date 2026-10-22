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
    QTest::addColumn<QString>("warningMsg");

    QTest::newRow("Short") << QByteArrayLiteral("\x70\x60\x7f") << 0 << char(0) << char(0)
                           << "Reading from sysex file ended prematurely at position 3";

    QByteArray long_buf;
    for (char c = 0; c < 100; ++c) {
        long_buf.append(c);
    }
    QTest::newRow("Long") << long_buf << 66 << char(0) << char(65) << QString();
}

void TestUtils::test_sysexTextFile()
{
    QTemporaryFile f;
    if (!f.open()) {
        QFAIL("Cannot open temporary file");
    }
    const QString p = f.fileName();
    f.close();

    // QFETCH declares the variable itself, so it cannot be made const.
    // NOLINTBEGIN(misc-const-correctness)
    QFETCH(QByteArray, bytes);
    QFETCH(int, readCount);
    QFETCH(char, readBegin);
    QFETCH(char, readEnd);
    QFETCH(QString, warningMsg);
    // NOLINTEND(misc-const-correctness)

    writeProgramFile(bytes, p);

    if (!warningMsg.isNull()) {
        QTest::ignoreMessage(QtWarningMsg, warningMsg.toLatin1());
    }
    const QByteArray read = fromSysexTextFile(p);
    QCOMPARE(read.size(), readCount);

    if (readCount > 0) {
        QCOMPARE(read.at(0), readBegin);
        QCOMPARE(read.at(readCount - 1), readEnd);
    }
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

    QTest::ignoreMessage(QtWarningMsg, QRegularExpression("Invalid byte in sysex file"));
    QCOMPARE(fromSysexTextFile(f.fileName()), QByteArray());
}

QTEST_APPLESS_MAIN(TestUtils)

#include "utils_test.moc"
