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
    # platform plugin lets future widget tests run headless too. TIMEOUT
    # makes a hung test fail quickly instead of after CTest's default
    # 25 minutes.
    set_tests_properties(${_target}
        PROPERTIES
            ENVIRONMENT QT_QPA_PLATFORM=offscreen
            TIMEOUT 60
    )
endfunction()

_add_test(commands)
_add_test(db)
_add_test(iomidi)
_add_test(lpd8_sysex)
_add_test(mainwindow)
_add_test(midiconnectionsmodel)
_add_test(midivaluedelegate)
_add_test(programsmodel)
_add_test(utils)
