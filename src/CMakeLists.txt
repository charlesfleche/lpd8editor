configure_file(app_info.h.in ${CMAKE_CURRENT_BINARY_DIR}/app_info.h)

set(_exe ${CMAKE_PROJECT_NAME})
set(_lib ${CMAKE_PROJECT_NAME}-lib)

qt5_create_translation(_qm_files
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/translations/lpd8editor.el.ts
    ${CMAKE_SOURCE_DIR}/translations/lpd8editor.fr.ts
    ${CMAKE_SOURCE_DIR}/translations/lpd8editor.it.ts
)
qt5_add_translation(_qm_files
    ${CMAKE_SOURCE_DIR}/translations/lpd8editor.el.ts
    ${CMAKE_SOURCE_DIR}/translations/lpd8editor.fr.ts
    ${CMAKE_SOURCE_DIR}/translations/lpd8editor.it.ts
)

install(
  FILES
    ${_qm_files}
  DESTINATION
    "${CMAKE_INSTALL_FULL_DATAROOTDIR}/${CMAKE_PROJECT_NAME}/${CMAKE_PROJECT_NAME}"
)

qt5_add_resources(_rcc_sources
    sql.qrc
)

add_library(${_lib}
    commands.cpp
    commands.h
    create_table_knobs.sql
    create_table_pads.sql
    create_table_programs.sql
    create_trigger_add_program.sql
    create_trigger_delete_program.sql
    db.cpp
    db.h
    enums.h
    iomidi.cpp
    iomidi.h
    lpd8editor.cpp
    lpd8editor.h
    lpd8_sysex.cpp
    lpd8_sysex.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    midiconnectionsmodel.cpp
    midiconnectionsmodel.h
    midivaluedelegate.cpp
    midivaluedelegate.h
    programsmodel.cpp
    programsmodel.h
    sql.qrc
    sysexhandler.cpp
    sysexhandler.h
    utils.cpp
    utils.h
    ${_rcc_sources}
    ${_qm_files}
)

target_include_directories(${_lib}
    SYSTEM
    PRIVATE
        ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(${_lib}
    Qt5::Sql
    Qt5::Svg
    Qt5::Widgets
    ${ALSA_LIBRARIES}
)

add_executable(${_exe}
    main.cpp
    app_info.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/app_info.h
)

target_include_directories(${_exe}
    PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(${_exe}
    ${_lib}
)

install(
  TARGETS
    ${_exe}
  DESTINATION
    "${CMAKE_INSTALL_FULL_BINDIR}"
)

function(_add_test _name)
    set(_target ${_name}_test)
    add_executable(${_target}
        ${_target}.cpp
    )
    target_link_libraries(${_target}
        Qt5::Test
        ${_lib}
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
