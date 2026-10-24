// lpd8::SysexAssembler: what turns the bytes another MIDI client sends into
// the messages the application handles (AlsaSeqTransport). The first input
// byte picks the size limit (small, so that fuzzing reaches it) and the
// second how the rest is cut into pieces, as a backend may deliver one
// message in several events. Whatever comes, the assembler must:
//
// - give only complete messages: F0, data bytes, F7, within the limit;
// - never hold more than the limit;
// - give the same messages however the bytes are cut.

#include "fuzzer.h"

#include "miditransport.h"
#include "sysexassembler.h"

#include <QByteArray>
#include <QList>

#include <algorithm>

namespace {

struct Outcome
{
    QList<QByteArray> messages;
    int aborted{0};
};

// How the input is fed: the assembler's size limit, and the size of the
// pieces it is cut into.
struct Cut
{
    qsizetype maxSize;
    qsizetype piece;
};

Outcome feedInPieces(QByteArrayView bytes, Cut cut)
{
    const qsizetype maxSize = cut.maxSize;
    const qsizetype piece = cut.piece;
    lpd8::SysexAssembler assembler(maxSize);
    Outcome outcome;
    for (qsizetype at = 0; at < bytes.size(); at += piece) {
        const auto result = assembler.feed(bytes.sliced(at, std::min(piece, bytes.size() - at)));
        outcome.messages += result.messages;
        outcome.aborted += result.aborted;
        check(assembler.buffered() <= maxSize);
    }
    return outcome;
}

} // namespace

extern "C" int LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/)
{
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
    const QByteArrayView all = input(data, size);
    if (all.size() < 2) {
        return 0;
    }
    const qsizetype maxSize = 2 + (static_cast<unsigned char>(all[0]) % 80);
    const qsizetype piece = 1 + (static_cast<unsigned char>(all[1]) % 16);
    const QByteArrayView bytes = all.sliced(2);

    // At once: one piece as large as the input.
    const Outcome whole = feedInPieces(bytes, {.maxSize = maxSize, .piece = bytes.size() + 1});
    for (const QByteArray &message : whole.messages) {
        check(lpd8::MidiTransport::isCompleteSysex(message));
        check(message.size() <= maxSize);
    }

    const Outcome pieces = feedInPieces(bytes, {.maxSize = maxSize, .piece = piece});
    check(pieces.messages == whole.messages);
    check(pieces.aborted == whole.aborted);
    return 0;
}
