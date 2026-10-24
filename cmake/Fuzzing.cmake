# libFuzzer builds, for the fuzzers in fuzz/.
#
# Usage: cmake --preset clang-fuzz (Clang only: GCC has no libFuzzer)
#
# Everything is compiled with coverage instrumentation
# (-fsanitize=fuzzer-no-link), so that libFuzzer sees which code each input
# reaches, including the library code under test. The preset also turns on
# ASan and UBSan, which turn a silent memory error into a crash that
# libFuzzer reports and saves.

option(LPD8EDITOR_FUZZ "Build the libFuzzer fuzzers (Clang only)" OFF)

if (LPD8EDITOR_FUZZ)
    if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message(FATAL_ERROR "LPD8EDITOR_FUZZ needs Clang: GCC has no libFuzzer")
    endif()
    add_compile_options(-fsanitize=fuzzer-no-link)
endif()
