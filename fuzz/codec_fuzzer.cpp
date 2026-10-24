// SysEx program messages: lpd8::decodeProgram(), which reads what comes
// from the device (Lpd8Device), from program files and from the QSettings
// template. Decoding must never crash, an error must point inside the
// message, and what it accepts must be a program the rest of the
// application can rely on: valid, and encoded back to the same bytes.

#include "fuzzer.h"

#include "deviceprofile.h"
#include "lpd8codec.h"
#include "miditransport.h"

#include <QByteArray>

extern "C" int LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/)
{
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
    const QByteArray bytes = input(data, size).toByteArray();
    const auto decoded = lpd8::decodeProgram(bytes);

    if (!decoded) {
        // An error says where, inside the message, or -1.
        check(decoded.error().offset >= -1 && decoded.error().offset < bytes.size());
        check(!lpd8::describe(decoded.error()).isEmpty());
        return 0;
    }

    // Accepted: a whole MIDI SysEx message, with data bytes only inside.
    check(lpd8::MidiTransport::isCompleteSysex(bytes));
    const lpd8::Program &program = decoded->program;
    check(program.isValid());
    check(decoded->slot >= 1 && decoded->slot <= lpd8::profile().programSlots);

    // What is decoded encodes back to the same program, in each format...
    const QByteArray stored = lpd8::encodeStoredProgram(program);
    const auto storedAgain = lpd8::decodeProgram(stored);
    check(storedAgain.has_value() && storedAgain->program == program);

    const auto request = lpd8::encodeSendProgram(program, decoded->slot);
    check(request.has_value());
    check(lpd8::MidiTransport::isCompleteSysex(*request));
    const auto requestAgain = lpd8::decodeProgram(*request);
    check(requestAgain.has_value() && requestAgain->program == program
          && requestAgain->slot == decoded->slot);

    // ...and only the header differs between the formats: the body is the
    // input's, byte for byte (nothing is normalized on the way).
    check(stored.sliced(8) == bytes.sliced(8));
    check(request->sliced(8) == bytes.sliced(8));
    return 0;
}
