#ifndef FUZZER_H
#define FUZZER_H

#include <QByteArrayView>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <print>
#include <source_location>

// The libFuzzer interface that every fuzzer of this folder implements. The
// same code is linked either with libFuzzer (LPD8EDITOR_FUZZ=ON, Clang), to
// fuzz, or with replay_main.cpp, which runs the files of a corpus once: that
// is how the tests run the seeds with any compiler.
extern "C" int LLVMFuzzerInitialize(int *argc, char ***argv);
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t *data, std::size_t size);

// The fuzzer input as a view on its bytes. libFuzzer gives exactly 'size'
// bytes, without a terminating NUL: a parser that reads one byte too far
// is caught by ASan.
inline QByteArrayView input(const std::uint8_t *data, std::size_t size)
{
    return {data, static_cast<qsizetype>(size)};
}

// Fails the run when a property doesn't hold, saying where. Unlike
// Q_ASSERT, it is also checked in release builds, and libFuzzer saves the
// input that broke it.
inline void check(bool property, std::source_location where = std::source_location::current())
{
    if (!property) {
        std::println(stderr, "{}:{}: property failed", where.file_name(), where.line());
        std::abort();
    }
}

#endif // FUZZER_H
