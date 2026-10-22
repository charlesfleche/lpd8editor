# clang-format targets:
#
#   cmake --build <dir> --target format        reformat the sources in place
#   cmake --build <dir> --target check-format  fail if a file is not formatted
#
# The style is in .clang-format at the top of the source tree. Different
# clang-format releases format slightly differently, so CI checks the
# formatting with the version from Debian trixie (19).

find_program(CLANG_FORMAT_EXECUTABLE NAMES clang-format clang-format-19)

if (NOT CLANG_FORMAT_EXECUTABLE)
    message(STATUS "clang-format not found: the format targets are not available")
    return()
endif()

# CONFIGURE_DEPENDS: CMake checks the glob again at each build, so a new
# source file is picked up without re-running cmake by hand.
file(GLOB_RECURSE _format_sources CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/tests/*.cpp"
    "${CMAKE_SOURCE_DIR}/tests/*.h"
)

add_custom_target(format
    COMMAND ${CLANG_FORMAT_EXECUTABLE} -i ${_format_sources}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting the sources with clang-format"
    VERBATIM
)

add_custom_target(check-format
    COMMAND ${CLANG_FORMAT_EXECUTABLE} --dry-run --Werror ${_format_sources}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking the formatting with clang-format"
    VERBATIM
)
