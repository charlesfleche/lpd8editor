function(_add_test _name)
    set(_target ${_name}_test)
    add_executable(${_target}
        ${_target}.cpp
    )
    target_link_libraries(${_target}
        Qt::Test
        ${CMAKE_PROJECT_NAME}-lib
    )
    add_test(
        NAME ${_target}
        COMMAND ${_target}
    )
endfunction()

_add_test(commands)
_add_test(db)
_add_test(iomidi)
_add_test(lpd8_sysex)
_add_test(midivaluedelegate)
_add_test(programsmodel)
_add_test(utils)
