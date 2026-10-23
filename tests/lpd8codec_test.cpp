#include "deviceprofile.h"
#include "lpd8codec.h"

#include "test_sysex.h"

#include <QtTest>

using namespace lpd8;

class TestLpd8Codec : public QObject
{
    Q_OBJECT

private slots:
    void test_decodeStoredDefault();
    void test_encodeStoredIsByteCompatible();
    void test_roundTrip();
    void test_decodeDocumentedDump();
    void test_encodeGetProgram();
    void test_encodeSendProgram();
    void test_encodersRefuse_data();
    void test_encodersRefuse();
    void test_decodeRejects_data();
    void test_decodeRejects();
    void test_describeNamesThePosition();
};

void TestLpd8Codec::test_decodeStoredDefault()
{
    // default_sysex is what the SQL storage produced for a new program.
    const auto decoded = decodeProgram(default_sysex);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->slot, 1);
    QCOMPARE(decoded->program, profile().defaultProgram);
    QCOMPARE(decoded->kind, DecodedProgram::Kind::Stored);
}

void TestLpd8Codec::test_encodeStoredIsByteCompatible()
{
    // Files exported by earlier versions must stay identical.
    QCOMPARE(encodeStoredProgram(profile().defaultProgram), default_sysex);
    // Program{} is not all zeros (a knob's range defaults to 0-127), so
    // the all-zero program comes from decoding.
    const auto zero = decodeProgram(allzero_sysex);
    QVERIFY(zero.has_value());
    QCOMPARE(encodeStoredProgram(zero->program), allzero_sysex);
}

void TestLpd8Codec::test_roundTrip()
{
    Program p = profile().defaultProgram;
    p.channel = 15;
    p.pads[2].toggle = true;
    p.pads[7].note = 127;
    p.knobs[5] = Knob{.controlChange = 1, .low = 10, .high = 100};

    const auto message = encodeSendProgram(p, 3);
    QVERIFY(message.has_value());
    const auto decoded = decodeProgram(*message);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->slot, 3);
    QCOMPARE(decoded->program, p);
}

void TestLpd8Codec::test_decodeDocumentedDump()
{
    // The response example of doc/SYSEX.md, from a real device.
    const QByteArray dump = QByteArray::fromHex(
            "F0477F7563003A01060C0D0E000F101100121314011516170000010200030405"
            "0006070800090A0B0018191A1B1C1D1E1F202122232425262728292A2B2C2D2E2FF7");
    const auto decoded = decodeProgram(dump);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->slot, 1);
    QCOMPARE(decoded->kind, DecodedProgram::Kind::Dump);
    QCOMPARE(decoded->program.channel, quint8(6));
    QCOMPARE(decoded->program.pads[0],
             (Pad{.note = 0x0c, .programChange = 0x0d, .controlChange = 0x0e, .toggle = false}));
    QCOMPARE(decoded->program.pads[2].toggle, true);
    QCOMPARE(decoded->program.knobs[7], (Knob{.controlChange = 0x2d, .low = 0x2e, .high = 0x2f}));
}

void TestLpd8Codec::test_encodeGetProgram()
{
    // The request example of doc/SYSEX.md.
    QCOMPARE(encodeGetProgram(1), QByteArray::fromHex("F0477F7563000101F7"));
}

void TestLpd8Codec::test_encodeSendProgram()
{
    const auto message = encodeSendProgram(profile().defaultProgram, 4);
    QVERIFY(message.has_value());
    QCOMPARE(message->size(), program_message_size);
    QCOMPARE(message->first(8), QByteArray::fromHex("F0477F7561003A04"));
    const auto decoded = decodeProgram(*message);
    QVERIFY(decoded.has_value());
    QCOMPARE(decoded->kind, DecodedProgram::Kind::SendRequest);
    QCOMPARE(decoded->slot, 4);
}

void TestLpd8Codec::test_encodersRefuse_data()
{
    QTest::addColumn<Program>("program");
    QTest::addColumn<int>("slot");
    QTest::addColumn<EncodeError>("error");

    const Program valid = profile().defaultProgram;
    QTest::newRow("Slot 0") << valid << 0 << EncodeError::InvalidSlot;
    QTest::newRow("Slot 5") << valid << 5 << EncodeError::InvalidSlot;
    QTest::newRow("Slot 300") << valid << 300 << EncodeError::InvalidSlot;

    Program p = valid;
    p.channel = 200;
    QTest::newRow("Channel 200") << p << 1 << EncodeError::InvalidProgram;
    p = valid;
    p.pads[4].note = 200;
    QTest::newRow("Note 200") << p << 1 << EncodeError::InvalidProgram;
}

void TestLpd8Codec::test_encodersRefuse()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(Program, program);
    QFETCH(int, slot);
    QFETCH(EncodeError, error);
    // NOLINTEND(misc-const-correctness)

    // Their output goes to real hardware: no malformed message must leave.
    const auto send = encodeSendProgram(program, slot);
    QVERIFY(!send.has_value());
    QCOMPARE(send.error(), error);
    if (error == EncodeError::InvalidSlot) {
        const auto get = encodeGetProgram(slot);
        QVERIFY(!get.has_value());
        QCOMPARE(get.error(), EncodeError::InvalidSlot);
    }
}

void TestLpd8Codec::test_decodeRejects_data()
{
    QTest::addColumn<QByteArray>("sysex");
    QTest::addColumn<DecodeError>("error");

    using Kind = DecodeError::Kind;
    const auto withByte = [](qsizetype offset, char value) {
        QByteArray s = default_sysex;
        s[offset] = value;
        return s;
    };
    const auto notAProgram = [](qsizetype offset) {
        return DecodeError{.kind = Kind::NotAProgram, .offset = offset};
    };
    const auto outOfRange = [](qsizetype offset) {
        return DecodeError{.kind = Kind::ValueOutOfRange, .offset = offset};
    };

    QTest::newRow("Empty") << QByteArray() << notAProgram(-1);
    QTest::newRow("Truncated") << default_sysex.first(65) << notAProgram(-1);
    QTest::newRow("No start byte") << withByte(0, '\x00') << notAProgram(0);
    QTest::newRow("No end byte") << withByte(65, '\x00') << notAProgram(65);
    QTest::newRow("Not Akai") << withByte(1, '\x41') << notAProgram(1);
    QTest::newRow("Not an LPD8") << withByte(3, '\x76') << notAProgram(1);
    QTest::newRow("Unknown opcode, 1st byte") << withByte(4, '\x10') << notAProgram(4);
    // Only the last opcode byte differs from a dump (63 00 3A): every byte
    // of the opcode must be compared.
    QTest::newRow("Unknown opcode, 3rd byte") << withByte(6, '\x3b') << notAProgram(4);
    QTest::newRow("Slot 0") << withByte(7, '\x00') << notAProgram(7);
    QTest::newRow("Slot 5") << withByte(7, '\x05') << notAProgram(7);
    QTest::newRow("Slot 0x80") << withByte(7, '\x80') << notAProgram(7);
    QTest::newRow("Channel 17") << invalid_sysex_channel << outOfRange(8);
    QTest::newRow("Channel 0x80") << withByte(8, '\x80') << outOfRange(8);
    QTest::newRow("Note above 127") << withByte(37, '\x80') << outOfRange(37);
    QTest::newRow("Toggle 2") << withByte(12, '\x02') << outOfRange(12);
    QTest::newRow("Knob high 0xff") << withByte(64, '\xff') << outOfRange(64);
}

void TestLpd8Codec::test_decodeRejects()
{
    // NOLINTBEGIN(misc-const-correctness): QFETCH declares the variables
    QFETCH(QByteArray, sysex);
    QFETCH(DecodeError, error);
    // NOLINTEND(misc-const-correctness)

    const auto decoded = decodeProgram(sysex);
    QVERIFY(!decoded.has_value());
    QCOMPARE(decoded.error(), error);
}

void TestLpd8Codec::test_describeNamesThePosition()
{
    // Byte 37 (0-based) is value number 38 for a person counting from 1.
    QByteArray sysex = default_sysex;
    sysex[37] = '\x80';
    const auto decoded = decodeProgram(sysex);
    QVERIFY(!decoded.has_value());
    QVERIFY2(describe(decoded.error()).contains("position 38"),
             qPrintable(describe(decoded.error())));
}

QTEST_APPLESS_MAIN(TestLpd8Codec)

#include "lpd8codec_test.moc"
