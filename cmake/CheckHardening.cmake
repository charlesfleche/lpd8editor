# Checks the hardening of a binary with readelf. Run by the 'hardening'
# test (cmake/Hardening.cmake):
#
#   cmake -DREADELF=<readelf> -DBINARY=<file> [-DEXPECT_FORTIFY=ON] -P CheckHardening.cmake
#
# It reads what the loader will do with the binary, so a flag lost to a
# toolchain or build change is caught, which reading the build files isn't.

function(_readelf _out)
    execute_process(
        COMMAND ${READELF} ${ARGN} ${BINARY}
        OUTPUT_VARIABLE _output
        RESULT_VARIABLE _result
        ERROR_VARIABLE _error
    )
    if (NOT _result EQUAL 0)
        message(FATAL_ERROR "readelf ${ARGN} ${BINARY} failed: ${_error}")
    endif()
    set(${_out} "${_output}" PARENT_SCOPE)
endfunction()

set(_failures "")

_readelf(_header --file-header)
_readelf(_segments --program-headers --wide)
_readelf(_dynamic --dynamic)
_readelf(_symbols --dyn-syms --wide)

# PIE: a shared-object type executable, marked PIE.
if (NOT _header MATCHES "Type:[ ]+DYN" OR NOT _dynamic MATCHES "PIE")
    list(APPEND _failures "not a position-independent executable (PIE)")
endif()

# Full RELRO: a GNU_RELRO segment, and immediate binding.
if (NOT _segments MATCHES "GNU_RELRO")
    list(APPEND _failures "no RELRO segment")
endif()
if (NOT _dynamic MATCHES "BIND_NOW" AND NOT _dynamic MATCHES "FLAGS_1[^\n]*NOW")
    list(APPEND _failures "lazy binding (no BIND_NOW): RELRO is only partial")
endif()

# A non-executable stack: the GNU_STACK segment is RW, not RWE.
string(REGEX MATCH "GNU_STACK[^\n]*" _stack "${_segments}")
if (NOT _stack)
    list(APPEND _failures "no GNU_STACK segment: the stack may be executable")
elseif (_stack MATCHES "RWE")
    list(APPEND _failures "executable stack")
endif()

# Stack protector: the binary calls __stack_chk_fail.
if (NOT _symbols MATCHES "__stack_chk_fail")
    list(APPEND _failures "no stack protector (__stack_chk_fail is not used)")
endif()

# _FORTIFY_SOURCE, in the builds that have it (cmake/Hardening.cmake): the
# binary imports a checked function, such as __memset_chk. __stack_chk_fail
# is not one: the name must end in _chk.
if (EXPECT_FORTIFY AND NOT _symbols MATCHES "__[a-z0-9_]+_chk[@ \n]")
    list(APPEND _failures "no _FORTIFY_SOURCE (no __*_chk function is imported)")
endif()

if (_failures)
    list(JOIN _failures "\n  " _list)
    message(FATAL_ERROR "${BINARY} is missing hardening:\n  ${_list}")
endif()
message(STATUS "${BINARY}: PIE, full RELRO, non-executable stack, stack protector")
