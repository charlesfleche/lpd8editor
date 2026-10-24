#include "utils.h"

#include <QCoreApplication>
#include <QDir>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QTextStream>

#include <sys/stat.h>

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
    void test_sysexTextFileRefusesWhatIsNotAFile();
    void test_sysexTextFileRefusesLargeFiles();
    void test_parseProgramTextIsDecimal();

    void test_writeProgramFileReportsErrors();

    void test_fileNameFor_data();
    void test_fileNameFor();
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
    // QTextStream's '>> int' read all of these as numbers.
    QTest::newRow("Hexadecimal") << valid + "0x10";
    QTest::newRow("Binary") << valid + "0b101";
    QTest::newRow("Plus sign") << valid + "+5";
    QTest::newRow("Wraps around to 5") << valid + "4294967301";
    QTest::newRow("Four digits") << valid + "0255";
    QTest::newRow("Exponent") << valid + "1e2";
    QTest::newRow("NUL byte") << valid + QByteArray("1\0", 2);
    QTest::newRow("Non-breaking space") << valid
                    + "\xc2\xa0"
                      "1";
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
    QVERIFY2(read.error().contains("invalid value at position 66"), qPrintable(read.error()));
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

void TestUtils::test_sysexTextFileRefusesWhatIsNotAFile()
{
    // Opening a FIFO waits for a writer: importing one froze the window
    // forever (this test hung before the fix).
    const QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const QString fifo = dir.filePath("program.lpd8");
    QCOMPARE(::mkfifo(QFile::encodeName(fifo).constData(), 0600), 0);

    auto read = fromSysexTextFile(fifo);
    QVERIFY(!read.has_value());
    QVERIFY2(read.error().endsWith("is not a file."), qPrintable(read.error()));

    read = fromSysexTextFile(dir.path());
    QVERIFY(!read.has_value());
}

void TestUtils::test_sysexTextFileRefusesLargeFiles()
{
    // A valid program after a lot of whitespace: refused without reading
    // the whole file.
    QTemporaryFile f;
    QVERIFY(f.open());
    f.write(QByteArray(max_program_file_size, ' '));
    for (int i = 0; i < 66; ++i) {
        f.write("1 ");
    }
    f.close();

    const auto read = fromSysexTextFile(f.fileName());
    QVERIFY(!read.has_value());
    QVERIFY2(read.error().contains("too large"), qPrintable(read.error()));
}

void TestUtils::test_parseProgramTextIsDecimal()
{
    QByteArray text;
    for (int i = 0; i < 66; ++i) {
        text += i % 2 == 0 ? "010\t" : "255\r\n";
    }
    const auto parsed = parseProgramText(text, "p");
    QVERIFY(parsed.has_value());
    QCOMPARE(parsed->at(0), '\x0a'); // 10: QTextStream read it as octal 8
    QCOMPARE(parsed->at(1), '\xff');

    // Every prefix without the last value is too short: never a crash,
    // nor a read past its end (the view has no terminating NUL).
    const qsizetype lastValue = text.size() - 5; // where the final "255\r\n" starts
    for (qsizetype n = 0; n <= lastValue; ++n) {
        QVERIFY(!parseProgramText(QByteArrayView(text).first(n), "p").has_value());
    }
}

void TestUtils::test_writeProgramFileReportsErrors()
{
    const auto written = writeProgramFile(QByteArray(66, 0), "/this/folder/does/not/exist/p.lpd8");
    QVERIFY(!written.has_value());
    QVERIFY2(written.error().startsWith("Cannot write"), qPrintable(written.error()));
}

void TestUtils::test_fileNameFor_data()
{
    QTest::addColumn<QString>("name");
    QTest::addColumn<QString>("expected");

    QTest::newRow("Plain") << "Drums" << "Drums";
    QTest::newRow("Unicode") << "Batterie é ♯" << "Batterie é ♯";
    QTest::newRow("Up the tree") << "../../.bashrc" << "_.._.bashrc";
    QTest::newRow("Absolute") << "/etc/passwd" << "_etc_passwd";
    QTest::newRow("Backslash") << "a\\b" << "a_b";
    QTest::newRow("Hidden") << ".profile" << "profile";
    QTest::newRow("Only dots") << ".." << "";
    // Spaces first: trimmed after the dots were dropped, they left a dot
    // in front, a hidden file.
    QTest::newRow("Space, then hidden") << " .bashrc" << "bashrc";
    QTest::newRow("Space, then dot") << " .x" << "x";
    QTest::newRow("Dots and spaces") << ". . .x" << "x";
    QTest::newRow("Newline") << "a\nb" << "a_b";
    QTest::newRow("Right-to-left override") << QStringLiteral("a\u202Eb") << "a_b";
    QTest::newRow("Empty") << "" << "";
}

void TestUtils::test_fileNameFor()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(QString, name);
    QFETCH(QString, expected);
    // NOLINTEND(misc-const-correctness)

    const QString fileName = fileNameFor(name);
    QCOMPARE(fileName, expected);
    // Never a hidden file, nor '..', nor surrounding spaces.
    QVERIFY(!fileName.startsWith(u'.'));
    QCOMPARE(fileName, fileName.trimmed());
    // Whatever the name, the file stays in the folder.
    const QDir dir("/home/user/Documents");
    if (!fileName.isEmpty()) {
        QCOMPARE(QFileInfo(dir.absoluteFilePath(fileName + ".lpd8")).absolutePath(),
                 dir.absolutePath());
    }
}

QTEST_APPLESS_MAIN(TestUtils)

#include "utils_test.moc"
