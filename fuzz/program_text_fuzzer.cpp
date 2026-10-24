// Program files: parseProgramText(), what 'Import program' reads.

#include "fuzzer.h"

#include "utils.h"

#include <QByteArray>
#include <QString>

extern "C" int LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/)
{
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
    const auto parsed = parseProgramText(input(data, size), QStringLiteral("fuzz"));
    if (!parsed) {
        check(!parsed.error().isEmpty());
        return 0;
    }

    // An accepted text holds only ASCII digits and whitespace, with at most
    // 3 digits per value: nothing a stricter reader would read differently
    // ('0x10', '+5', '0255', a number that wraps around).
    qsizetype digits = 0;
    for (const char c : input(data, size)) {
        if (c >= '0' && c <= '9') {
            check(++digits <= 3);
            continue;
        }
        check(c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f');
        digits = 0;
    }

    // A file that parses holds exactly one program's worth of bytes...
    check(parsed->size() == 66);

    // ...and writing it in the export format (decimal values, each followed
    // by a space) and parsing that gives the same bytes back.
    QByteArray text;
    for (const char byte : *parsed) {
        text += QByteArray::number(static_cast<unsigned char>(byte)) + ' ';
    }
    const auto again = parseProgramText(text, QStringLiteral("fuzz"));
    check(again.has_value() && *again == *parsed);
    return 0;
}
