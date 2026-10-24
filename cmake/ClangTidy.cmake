# clang-tidy, the clang static analyzer and linter. Checks are in .clang-tidy.
#
# Two ways to run it:
#
# - LPD8EDITOR_CLANG_TIDY=ON runs clang-tidy on each file as it compiles, and
#   reports the findings as compiler warnings (also shown in IDEs). This
#   slows the build down a lot, so it is off by default.
#
# - The 'clang-tidy' target checks the whole tree in parallel with
#   run-clang-tidy. It needs compile_commands.json, which the presets turn
#   on. Use the Clang presets: clang-tidy doesn't know GCC-only flags.
#
# Different clang-tidy releases report different things, so CI uses the
# version from Debian trixie (19).

option(LPD8EDITOR_CLANG_TIDY "Run clang-tidy on each file while building" OFF)

# Prefer the versioned names, as for clang-format: CI uses clang-tidy 19, and
# another release reports other findings.
find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy-19 clang-tidy)
find_program(RUN_CLANG_TIDY_EXECUTABLE NAMES run-clang-tidy-19 run-clang-tidy)

if (CLANG_TIDY_EXECUTABLE)
    execute_process(
        COMMAND ${CLANG_TIDY_EXECUTABLE} --version
        OUTPUT_VARIABLE _clang_tidy_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if (NOT _clang_tidy_version MATCHES "version 19\\.")
        message(WARNING
            "${CLANG_TIDY_EXECUTABLE} is not clang-tidy 19. CI runs clang-tidy 19, "
            "and other versions report different findings.")
    endif()
endif()

# Which files are ours, as regular expressions anchored on the source dir.
# The path is escaped first: a folder such as lpd8editor-0.0.18 is full of
# '.', which a regex would read as 'any character'.
#
#   header filter: ^<source>/(src|tests)/[^/]*\.h$
#   file filter:   ^<source>/(src|tests)/[^/]*\.cpp$
#
# They match files directly in src/ and tests/ only, so what the build
# generates is excluded even when the build folder is in the source tree
# (<source>/build/<preset>/src/app_info.h, .../qrc_sql.cpp, the *_autogen/
# folders). These command-line filters override HeaderFilterRegex from
# .clang-tidy, whose looser pattern is for editors.
string(REGEX REPLACE "([][.*+?^$(){}|\\])" "\\\\\\1" _source_regex "${CMAKE_SOURCE_DIR}")
set(_tidy_header_filter "^${_source_regex}/(src|tests|fuzz)/[^/]*\\.h$")
set(_tidy_file_filter "^${_source_regex}/(src|tests|fuzz)/[^/]*\\.cpp$")

if (LPD8EDITOR_CLANG_TIDY)
    if (NOT CLANG_TIDY_EXECUTABLE)
        message(FATAL_ERROR "LPD8EDITOR_CLANG_TIDY is ON but clang-tidy was not found")
    endif()
    # Must be set before the targets are created (add_subdirectory).
    set(CMAKE_CXX_CLANG_TIDY ${CLANG_TIDY_EXECUTABLE} "--header-filter=${_tidy_header_filter}")
endif()

if (RUN_CLANG_TIDY_EXECUTABLE)
    add_custom_target(clang-tidy
        COMMAND ${RUN_CLANG_TIDY_EXECUTABLE}
            -p ${CMAKE_BINARY_DIR}
            -quiet
            # Any finding fails the target (and CI), as with cppcheck.
            -warnings-as-errors=*
            -header-filter=${_tidy_header_filter}
            ${_tidy_file_filter}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on the sources"
        VERBATIM
    )
else()
    message(STATUS "run-clang-tidy not found: the clang-tidy target is not available")
endif()
