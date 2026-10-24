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

#include <algorithm>

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

    // What the store promises (programstore.h): valid programs and names,
    // unique ids from 1 to max_program_id, and a next id above all of them.
    for (const lpd8::StoredProgram &stored : content->programs) {
        check(stored.program.isValid());
        check(stored.name.isValidUtf16());
        check(stored.id >= 1 && stored.id <= lpd8::max_program_id);
        check(stored.id < content->nextId);
        check(std::ranges::count(content->programs, stored.id, &lpd8::StoredProgram::id) == 1);
    }
    check(content->nextId >= 1 && content->nextId <= lpd8::max_program_id + 1);

    // A save then a load gives the same store back.
    const auto again = lpd8::fromJson(lpd8::toJson(*content));
    check(again.has_value());
    check(again->programs == content->programs);
    check(again->nextId == content->nextId);
    return 0;
}
