# Compiler warnings shared by every target of the project.
#
# Usage: target_link_libraries(<target> PRIVATE lpd8editor_warnings)
#
# An INTERFACE library carries no code, only usage requirements (here,
# compile options) for the targets that link it. Linking it PRIVATE gives
# a target the warnings without passing them on to its users. Unlike
# setting CMAKE_CXX_FLAGS, this leaves code we don't own alone.
#
# Qt and ALSA headers come in through -isystem, so their own warnings are
# not reported.

option(LPD8EDITOR_WERROR "Treat compiler warnings as errors" OFF)

add_library(lpd8editor_warnings INTERFACE)

# GCC only. Clang already covers sign conversion with -Wconversion, and has
# its own versions of the others (or no equivalent).
set(_gcc_only_warnings
    -Wsign-conversion
    -Wmisleading-indentation
    -Wduplicated-cond
    -Wduplicated-branches
    -Wlogical-op
)

target_compile_options(lpd8editor_warnings
    INTERFACE
        # Understood by both GCC and Clang
        -Wall
        -Wextra
        -Wpedantic                  # non-standard extensions
        -Wshadow                    # a variable hides another one
        -Wnon-virtual-dtor          # polymorphic class without a virtual destructor
        -Wold-style-cast            # use static_cast and friends, which are greppable
        -Wcast-align
        -Woverloaded-virtual        # an overload hides a virtual function
        -Wconversion                # implicit narrowing (int -> char, qsizetype -> int)
        -Wnull-dereference
        -Wdouble-promotion
        -Wformat=2
        -Wimplicit-fallthrough      # a switch case falls through without [[fallthrough]]

        # A generator expression is one argument, so the list is expanded
        # inside quotes (CMake lists are ';'-separated).
        "$<$<CXX_COMPILER_ID:GNU>:${_gcc_only_warnings}>"

        $<$<BOOL:${LPD8EDITOR_WERROR}>:-Werror>
)
