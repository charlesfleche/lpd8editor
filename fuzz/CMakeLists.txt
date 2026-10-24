# Fuzzers for the code that reads what comes from outside: SysEx messages,
# program files, the programs file and the old database. See
# fuzz/README.md.
#
# Each fuzzer is built twice:
#
# - <name>_replay, in every build with tests: it runs the fuzzer once on
#   each file of its seed corpus (fuzz/corpus/<name>), with any compiler. The
#   test fuzz_<name> does that, so the properties the fuzzers check hold for
#   the seeds on every CI run.
# - <name>_fuzzer, with LPD8EDITOR_FUZZ=ON (Clang only): linked with
#   libFuzzer, to search for inputs that break those properties.

set(_fuzzers
    codec
    migration
    program_store
    program_text
    sysex_assembler
)

foreach(_name IN LISTS _fuzzers)
    set(_targets ${_name}_replay)
    add_executable(${_name}_replay
        ${_name}_fuzzer.cpp
        fuzzer.h
        replay_main.cpp
    )
    add_test(
        NAME fuzz_${_name}
        COMMAND ${_name}_replay ${CMAKE_CURRENT_SOURCE_DIR}/corpus/${_name}
    )
    set_tests_properties(fuzz_${_name}
        PROPERTIES
            ENVIRONMENT QT_QPA_PLATFORM=offscreen
            TIMEOUT ${LPD8EDITOR_TEST_TIMEOUT}
    )

    if (LPD8EDITOR_FUZZ)
        add_executable(${_name}_fuzzer
            ${_name}_fuzzer.cpp
            fuzzer.h
        )
        target_link_options(${_name}_fuzzer PRIVATE -fsanitize=fuzzer)
        list(APPEND _targets ${_name}_fuzzer)
    endif()

    foreach(_target IN LISTS _targets)
        target_link_libraries(${_target}
            PRIVATE
                ${CMAKE_PROJECT_NAME}-lib
                lpd8editor_warnings
        )
    endforeach()
endforeach()

# The migration fuzzer builds old databases itself.
target_link_libraries(migration_replay PRIVATE Qt::Sql)
if (LPD8EDITOR_FUZZ)
    target_link_libraries(migration_fuzzer PRIVATE Qt::Sql)
endif()
