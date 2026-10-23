#include "sysexassembler.h"

#include <QtTest>

using lpd8::SysexAssembler;

namespace {

QByteArray bytes(std::initializer_list<int> values)
{
    QByteArray result;
    for (const int v : values) {
        result += static_cast<char>(v);
    }
    return result;
}

} // namespace

class TestSysexAssembler : public QObject
{
    Q_OBJECT

private slots:
    void test_feed_data();
    void test_feed();
    void test_piecesMakeOneMessage();
    void test_tooLongIsDroppedAndReportedOnce();
};

void TestSysexAssembler::test_feed_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QList<QByteArray>>("expected");
    QTest::addColumn<int>("aborted");

    const QByteArray message = bytes({0xf0, 0x47, 0x7f, 0x75, 0xf7});

    QTest::newRow("One message") << message << QList<QByteArray>{message} << 0;
    QTest::newRow("Two messages") << message + message << QList<QByteArray>{message, message} << 0;
    QTest::newRow("Real time inside is skipped")
            << bytes({0xf0, 0x47, 0xf8, 0x7f, 0xfe, 0x75, 0xff, 0xf7}) << QList<QByteArray>{message}
            << 0;
    QTest::newRow("Real time outside is ignored")
            << bytes({0xf8, 0xfe}) + message + bytes({0xfa}) << QList<QByteArray>{message} << 0;
    QTest::newRow("A new F0 restarts")
            << bytes({0xf0, 0x01, 0x02}) + message << QList<QByteArray>{message} << 1;
    QTest::newRow("A status byte aborts")
            << bytes({0xf0, 0x01, 0x90, 0x3c, 0x40, 0xf7}) << QList<QByteArray>{} << 1;
    QTest::newRow("Every other status byte aborts") << bytes({0xf0, 0x01, 0xf6, 0xf7})
                    + bytes({0xf0, 0x01, 0xf1, 0xf7}) + bytes({0xf0, 0x01, 0x80, 0xf7})
                                                    << QList<QByteArray>{} << 3;
    QTest::newRow("Channel messages outside are ignored")
            << bytes({0x90, 0x3c, 0x40}) + message + bytes({0x80, 0x3c, 0x00})
            << QList<QByteArray>{message} << 0;
    QTest::newRow("F7 alone is ignored") << bytes({0xf7, 0x01}) << QList<QByteArray>{} << 0;
    QTest::newRow("Empty message")
            << bytes({0xf0, 0xf7}) << QList<QByteArray>{bytes({0xf0, 0xf7})} << 0;
    QTest::newRow("Unfinished: nothing yet") << bytes({0xf0, 0x01}) << QList<QByteArray>{} << 0;
}

void TestSysexAssembler::test_feed()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(QByteArray, input);
    QFETCH(QList<QByteArray>, expected);
    QFETCH(int, aborted);
    // NOLINTEND(misc-const-correctness)

    SysexAssembler assembler;
    const SysexAssembler::Result result = assembler.feed(input);
    QCOMPARE(result.messages, expected);
    QCOMPARE(result.aborted, aborted);
    QVERIFY(!result.tooLongReported);
}

void TestSysexAssembler::test_piecesMakeOneMessage()
{
    // The way a backend may deliver a long message: in several pieces.
    SysexAssembler assembler;
    QVERIFY(assembler.feed(bytes({0xf0, 0x47})).messages.isEmpty());
    QVERIFY(assembler.feed(bytes({0x7f})).messages.isEmpty());
    QCOMPARE(assembler.feed(bytes({0x75, 0xf7})).messages,
             QList<QByteArray>{bytes({0xf0, 0x47, 0x7f, 0x75, 0xf7})});
}

void TestSysexAssembler::test_tooLongIsDroppedAndReportedOnce()
{
    SysexAssembler assembler(8);
    const QByteArray fits = bytes({0xf0, 1, 2, 3, 4, 5, 6, 0xf7}); // exactly 8
    QCOMPARE(assembler.feed(fits).messages, QList<QByteArray>{fits});

    // One byte too many: dropped, and reported.
    SysexAssembler::Result result = assembler.feed(bytes({0xf0, 1, 2, 3, 4, 5, 6, 7, 0xf7}));
    QVERIFY(result.messages.isEmpty());
    QVERIFY(result.tooLongReported);

    // A source that keeps sending: still dropped, reported only once.
    result = assembler.feed(bytes({0xf0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0xf7}));
    QVERIFY(result.messages.isEmpty());
    QVERIFY(!result.tooLongReported);
    // An F0 with no F7 doesn't grow anything: the rest is skipped.
    result = assembler.feed(QByteArray(1, '\xf0') + QByteArray(100000, '\x01'));
    QVERIFY(result.messages.isEmpty());
    QVERIFY(!result.tooLongReported);
    QVERIFY(assembler.buffered() <= 8);

    // A message that fits gets through, and the next runaway one is
    // reported again.
    QCOMPARE(assembler.feed(fits).messages, QList<QByteArray>{fits});
    QVERIFY(assembler.feed(bytes({0xf0, 1, 2, 3, 4, 5, 6, 7, 0xf7})).tooLongReported);
}

QTEST_APPLESS_MAIN(TestSysexAssembler)

#include "sysexassembler_test.moc"
