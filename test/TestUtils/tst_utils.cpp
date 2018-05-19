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
    QVERIFY_EXCEPTION_THROWN(readTextFile("This path does not exist"),
                             std::runtime_error);
}

void TestUtils::test_sysexTextFile_data()
{
    QTest::addColumn<QByteArray>("bytes");
    QTest::addColumn<int>("readCount");
    QTest::addColumn<char>("readBegin");
    QTest::addColumn<char>("readEnd");
    QTest::addColumn<QString>("warningMsg");

    const char short_buf[] = {0x70, 0x60, 0x7F};
    QTest::newRow("Short") << QByteArray(short_buf, 3) << 0 << char(0) << char(0) << "Reading from sysex file ended prematurely at position 4";

    QByteArray long_buf;
    for (char c = 0 ; c < 100 ; ++c) {
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

    QFETCH(QByteArray, bytes);
    QFETCH(int, readCount);
    QFETCH(char, readBegin);
    QFETCH(char, readEnd);
    QFETCH(QString, warningMsg);

    writeProgramFile(bytes, p);

    if (!warningMsg.isNull()) {
        QTest::ignoreMessage(QtWarningMsg, warningMsg.toLatin1());
    }
    const QByteArray read = fromSysexTextFile(p);
    QCOMPARE(read.count(), readCount);

    if (readCount > 0) {
        QCOMPARE(read.at(0), readBegin);
        QCOMPARE(read.at(readCount-1), readEnd);
    }
}

QTEST_MAIN(TestUtils)

#include "tst_utils.moc"
