# Runtime sanitizers, for debugging and CI.
#
# Usage: cmake -DLPD8EDITOR_SANITIZERS="address;undefined" ...
#
# - address (ASan): out-of-bounds accesses, use-after-free, leaks (through
#   LeakSanitizer, which ASan includes).
# - undefined (UBSan): signed overflow, invalid casts, misaligned access...
#
# Unlike warnings, sanitizers are enabled for the whole build rather than
# per target: every object file and the final link must agree, or the
# reports are wrong or the link fails. ASan cannot be combined with
# valgrind, so use a separate build directory for each (see the presets).

set(LPD8EDITOR_SANITIZERS "" CACHE STRING
    "Semicolon-separated list of sanitizers to enable (address, undefined)")

if (LPD8EDITOR_SANITIZERS)
    list(JOIN LPD8EDITOR_SANITIZERS "," _sanitizers)
    message(STATUS "Sanitizers enabled: ${_sanitizers}")
    add_compile_options(
        -fsanitize=${_sanitizers}
        # Readable stack traces in the reports
        -fno-omit-frame-pointer
        # By default UBSan prints a message and continues, so the test
        # still passes. Abort instead, so undefined behaviour fails the test.
        -fno-sanitize-recover=all
    )
    add_link_options(-fsanitize=${_sanitizers})
endif()
