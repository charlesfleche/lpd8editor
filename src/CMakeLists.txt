configure_file(app_info.h.in ${CMAKE_CURRENT_BINARY_DIR}/app_info.h)

set(_exe ${CMAKE_PROJECT_NAME})
set(_lib ${CMAKE_PROJECT_NAME}-lib)

add_library(${_lib}
    deviceprofile.cpp
    deviceprofile.h
    enums.h
    iomidi.cpp
    iomidi.h
    lpd8codec.cpp
    lpd8codec.h
    lpd8_sysex.cpp
    lpd8_sysex.h
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    migration.cpp
    migration.h
    midinames.cpp
    midinames.h
    midiconnectionsmodel.cpp
    midiconnectionsmodel.h
    midivaluedelegate.cpp
    midivaluedelegate.h
    parameterlocation.h
    program.cpp
    program.h
    programfile.cpp
    programfile.h
    programmodels.cpp
    programmodels.h
    programsdocument.cpp
    programsdocument.h
    programstore.cpp
    programstore.h
    startup.cpp
    startup.h
    exactinteger.cpp
    exactinteger.h
    storecommands.cpp
    storecommands.h
    sysexhandler.cpp
    sysexhandler.h
    utils.cpp
    utils.h
)

# PUBLIC: everything linking the library (the app and the tests) is
# compiled as C++23 too.
target_compile_features(${_lib}
    PUBLIC
        cxx_std_23
)

# The tests include the library headers ("programstore.h", ...), so the source
# folder is a PUBLIC include directory.
target_include_directories(${_lib}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${_lib}
    PRIVATE
        lpd8editor_warnings
        # Only for the one-time migration from db.sqlite3 (migration.cpp):
        # no public header uses QtSql.
        Qt::Sql
    PUBLIC
        Qt::Widgets
        # Imported target from FindALSA: include path and library together.
        # PUBLIC because public headers (iomidi.h, midiconnectionsmodel.h)
        # use ALSA types, so whatever includes them needs the include path.
        ALSA::ALSA
)

add_executable(${_exe}
    main.cpp
    app_info.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/app_info.h
)

# The window icon, ':/lpd8editor.svg'. A resource of the executable, not of
# the static library: the linker keeps every object of an executable, but
# drops a library's resource object that no code references, so a library
# resource needs Q_INIT_RESOURCE().
qt_add_resources(${_exe} icon
    PREFIX "/"
    BASE ${CMAKE_SOURCE_DIR}
    FILES ${CMAKE_SOURCE_DIR}/lpd8editor.svg
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
