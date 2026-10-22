# Valgrind memcheck through CTest.
#
# Usage, from a build without sanitizers:
#   ctest -T memcheck --output-on-failure
#
# CTest runs every test under valgrind with the options below and writes
# one log per test in Testing/Temporary/MemoryChecker.*.log. It reports
# the number of defects per test.
#
# These variables must be set before include(CTest), which copies them
# into DartConfiguration.tcl, where 'ctest -T memcheck' reads them.

find_program(MEMORYCHECK_COMMAND valgrind)

set(MEMORYCHECK_COMMAND_OPTIONS
    # Report each leaked block with where it was allocated...
    "--leak-check=full"
    # ...but only count the 'definitely lost' ones as errors: Qt and fontconfig
    # keep global caches alive until exit, which show up as 'still reachable'
    # or 'possibly lost' without being bugs.
    "--show-leak-kinds=definite"
    "--errors-for-leak-kinds=definite"
    # For uses of uninitialized values, say where the value came from.
    "--track-origins=yes"
    # Print a ready-to-paste suppression for each error, to help maintain
    # the suppressions file.
    "--gen-suppressions=all"
    "--num-callers=30"
    # Make the test process fail when valgrind finds an error, so the test
    # (and CI) goes red. Otherwise CTest only lists the defects.
    "--error-exitcode=1"
)
string(JOIN " " MEMORYCHECK_COMMAND_OPTIONS ${MEMORYCHECK_COMMAND_OPTIONS})

set(MEMORYCHECK_SUPPRESSIONS_FILE "${CMAKE_SOURCE_DIR}/tools/valgrind.supp")
