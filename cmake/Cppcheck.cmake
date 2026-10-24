# cppcheck, a second static analyzer. It works differently from clang-tidy
# (its own parser and data-flow analysis), so it finds different things.
#
#   cmake --build <dir> --target cppcheck
#
# It reads the compile flags of each file from compile_commands.json (the
# presets turn it on) and fails the build on any finding.

find_program(CPPCHECK_EXECUTABLE cppcheck)

if (NOT CPPCHECK_EXECUTABLE)
    message(STATUS "cppcheck not found: the cppcheck target is not available")
    return()
endif()

add_custom_target(cppcheck
    COMMAND ${CPPCHECK_EXECUTABLE}
        --project=${CMAKE_BINARY_DIR}/compile_commands.json
        # Teaches cppcheck Qt's macros (Q_OBJECT, 'public slots:', emit...)
        --library=qt
        --enable=warning,style,performance,portability
        # The code base is small: analyze every branch, not only the first ones.
        --check-level=exhaustive
        # Allow '// cppcheck-suppress <id>' comments in the code.
        --inline-suppr
        --suppressions-list=${CMAKE_SOURCE_DIR}/tools/cppcheck-suppressions.txt
        # Our sources only. cppcheck matches the filter against the paths as
        # they appear in compile_commands.json (absolute here), and
        # <source>/src/* does not match the build folder's
        # <source>/build/<preset>/src/... files.
        --file-filter=${CMAKE_SOURCE_DIR}/src/*
        --file-filter=${CMAKE_SOURCE_DIR}/tests/*
        --file-filter=${CMAKE_SOURCE_DIR}/fuzz/*
        --error-exitcode=1
        --quiet
        -j 4
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running cppcheck on the sources"
    VERBATIM
)
