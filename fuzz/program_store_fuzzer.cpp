// The JSON programs file: lpd8::fromJson(), what ProgramStore::load()
// reads at every start. Whatever the file holds, reading it must not
// crash, and what it accepts must be a store the rest of the application
// can rely on.
//
// Only whether fromJson() succeeds is used, never its error: the error type
// is free to change (a message, or a kind and a message) without touching
// this fuzzer.

#include "fuzzer.h"

#include "programstore.h"

#include <QByteArray>

extern "C" int LLVMFuzzerInitialize(int * /*argc*/, char *** /*argv*/)
{
    return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size)
{
    const auto content = lpd8::fromJson(input(data, size).toByteArray());
    if (!content) {
        return 0;
    }

    // What the store promises (programstore.h): valid programs and names.
    for (const lpd8::NamedProgram &named : *content) {
        check(named.program.isValid());
        check(named.name.isValidUtf16());
    }

    // A save then a load gives the same store back.
    const auto again = lpd8::fromJson(lpd8::toJson(*content));
    check(again.has_value());
    check(*again == *content);
    return 0;
}
