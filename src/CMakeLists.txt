configure_file(app_info.h.in ${CMAKE_CURRENT_BINARY_DIR}/app_info.h)

set(_exe ${CMAKE_PROJECT_NAME})
set(_lib ${CMAKE_PROJECT_NAME}-lib)

qt_add_resources(_rcc_sources
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
    deviceprofile.cpp
    deviceprofile.h
    enums.h
    iomidi.cpp
    iomidi.h
    lpd8_sysex.cpp
    lpd8_sysex.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    midiconnectionsmodel.cpp
    midiconnectionsmodel.h
    midivaluedelegate.cpp
    midivaluedelegate.h
    program.cpp
    program.h
    programsmodel.cpp
    programsmodel.h
    sql.qrc
    sysexhandler.cpp
    sysexhandler.h
    utils.cpp
    utils.h
    ${_rcc_sources}
)

# PUBLIC: everything linking the library (the app and the tests) is
# compiled as C++23 too.
target_compile_features(${_lib}
    PUBLIC
        cxx_std_23
)

# The tests include the library headers ("db.h", ...), so the source
# folder is a PUBLIC include directory.
target_include_directories(${_lib}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(${_lib}
    SYSTEM
    PRIVATE
        ${ALSA_INCLUDE_DIRS}
)

target_link_libraries(${_lib}
    PRIVATE
        lpd8editor_warnings
    PUBLIC
        Qt::Sql
        Qt::Widgets
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
    PRIVATE
        ${_lib}
        lpd8editor_warnings
)

install(
  TARGETS
    ${_exe}
  DESTINATION
    "${CMAKE_INSTALL_BINDIR}"
)
