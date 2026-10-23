# Valgrind makes the tests 20 to 50 times slower: mainwindow_test takes
# about 7 s under memcheck on a desktop, and a shared CI runner can be
# several times slower than that. The valgrind preset raises the limit.
set(LPD8EDITOR_TEST_TIMEOUT 60 CACHE STRING "Time limit for each test, in seconds")

function(_add_test _name)
    set(_target ${_name}_test)
    add_executable(${_target}
        ${_target}.cpp
    )
    target_link_libraries(${_target}
        PRIVATE
            Qt::Test
            ${CMAKE_PROJECT_NAME}-lib
            lpd8editor_warnings
    )
    add_test(
        NAME ${_target}
        COMMAND ${_target}
    )
    # Tests must not need a display: CI machines have none. The offscreen
    # platform plugin lets widget tests run headless. TIMEOUT makes a hung
    # test fail instead of waiting for CTest's default of 25 minutes.
    set_tests_properties(${_target}
        PROPERTIES
            ENVIRONMENT QT_QPA_PLATFORM=offscreen
            TIMEOUT ${LPD8EDITOR_TEST_TIMEOUT}
    )
endfunction()

_add_test(iomidi)
_add_test(lpd8_sysex)
_add_test(lpd8codec)
_add_test(mainwindow)
_add_test(midiconnectionsmodel)
_add_test(migration)
_add_test(midivaluedelegate)
_add_test(program)
_add_test(programfile)
_add_test(programmodels)
_add_test(programsdocument)
_add_test(programstore)
_add_test(startup)
_add_test(storecommands)
_add_test(utils)

# These tests build old SQLite databases for the migration to read.
target_link_libraries(migration_test PRIVATE Qt::Sql)
target_link_libraries(startup_test PRIVATE Qt::Sql)
