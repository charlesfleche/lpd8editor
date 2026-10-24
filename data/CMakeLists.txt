# What a Linux desktop needs to show lpd8editor in its menus and software
# centers: a .desktop entry, the icon, and AppStream metadata. They are named
# after the application's id, io.github.charlesfleche.lpd8editor (reverse DNS
# of its home page), as the freedesktop.org and AppStream specifications ask.
set(_id io.github.charlesfleche.lpd8editor)

install(
  FILES ${_id}.desktop
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)
install(
  FILES ${_id}.metainfo.xml
  DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo
)
# One scalable icon is enough: every current desktop renders SVG (and the
# DEB depends on qt6-svg-plugins for the window icon anyway).
install(
  FILES ${CMAKE_SOURCE_DIR}/lpd8editor.svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
  RENAME ${_id}.svg
)

# Validate the files with the reference tools, as tests. A tool that isn't
# installed makes its test skipped (exit code 77), not failed, so a build
# without them still passes. CI configures with
# LPD8EDITOR_REQUIRE_VALIDATORS=ON, which makes a missing tool a failure:
# there, a skip would hide that the files were never checked.
option(LPD8EDITOR_REQUIRE_VALIDATORS
    "Fail, instead of skip, the validation tests whose tool is missing" OFF)
if (BUILD_TESTING)
    function(_add_validation _name _program)
        find_program(_tool_${_name} ${_program})
        if (_tool_${_name})
            add_test(NAME ${_name} COMMAND ${_tool_${_name}} ${ARGN})
        elseif (LPD8EDITOR_REQUIRE_VALIDATORS)
            add_test(NAME ${_name}
                COMMAND sh -c "echo '${_program} is not installed'; exit 1")
        else()
            add_test(NAME ${_name}
                COMMAND sh -c "echo '${_program} is not installed: skipped'; exit 77")
        endif()
        set_tests_properties(${_name} PROPERTIES SKIP_RETURN_CODE 77)
    endfunction()

    _add_validation(desktop_file_validate desktop-file-validate
        ${CMAKE_CURRENT_SOURCE_DIR}/${_id}.desktop)
    # --no-net: don't check that the URLs answer, so the test runs offline.
    _add_validation(appstream_validate appstreamcli
        validate --no-net ${CMAKE_CURRENT_SOURCE_DIR}/${_id}.metainfo.xml)

    # On a release tag (git describe gives exactly 'v<version>', with no
    # commit after it and no local change), the newest <release> of the
    # metainfo must be this version. Elsewhere the check is skipped: between
    # releases, the next version has no entry yet.
    if (LPD8EDITOR_QAPPLICATION_VERSION MATCHES "^v?[0-9]+[.][0-9]+[.][0-9]+-0-g[0-9a-f]+$")
        add_test(NAME metainfo_release_version
            COMMAND ${CMAKE_COMMAND}
                -DMETAINFO=${CMAKE_CURRENT_SOURCE_DIR}/${_id}.metainfo.xml
                -DVERSION=${PROJECT_VERSION}
                -P ${CMAKE_SOURCE_DIR}/cmake/CheckMetainfoRelease.cmake)
    else()
        add_test(NAME metainfo_release_version
            COMMAND sh -c "echo 'Not a release tag: skipped'; exit 77")
        set_tests_properties(metainfo_release_version PROPERTIES SKIP_RETURN_CODE 77)
    endif()
endif()

# Shown in IDEs.
add_custom_target(desktop-data SOURCES ${_id}.desktop ${_id}.metainfo.xml)
