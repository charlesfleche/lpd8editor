cmake_minimum_required(VERSION 3.10)


set(_project_version 0.0.0)
set(LPD8EDITOR_QAPPLICATION_VERSION ${_project_version})
find_package(Git)
if (GIT_EXECUTABLE)
    message(STATUS "Trying to get a version from git describe")
    execute_process(
        COMMAND
            ${GIT_EXECUTABLE} describe --tags --long --dirty --always
        WORKING_DIRECTORY
            ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE
            LPD8EDITOR_QAPPLICATION_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    string(REGEX MATCH [0-9]+\.[0-9]+\.[0-9]+
        _parsed_git_describe_version
        ${LPD8EDITOR_QAPPLICATION_VERSION}
    )
    message(STATUS "Git describe version: ${LPD8EDITOR_QAPPLICATION_VERSION}")
    message(STATUS "Parsed git describe version: ${_parsed_git_describe_version}")

    if (_parsed_git_describe_version VERSION_GREATER _project_version)
        set(_project_version ${_parsed_git_describe_version})
    endif()
endif()

project(lpd8editor
    VERSION ${_project_version}
    HOMEPAGE_URL https://github.com/charlesfleche/lpd8editor
    DESCRIPTION "A linux editor for the Akai LPD8"
    LANGUAGES CXX
)

message(STATUS "Project version: ${CMAKE_PROJECT_VERSION}")
message(STATUS "QApplication version: ${LPD8EDITOR_QAPPLICATION_VERSION}")

enable_testing()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

find_package(ALSA REQUIRED)
find_package(Git REQUIRED)
find_package(Qt5 COMPONENTS LinguistTools Sql Svg Test Widgets REQUIRED)

add_subdirectory(translations)
add_subdirectory(src)

add_custom_target(github
  SOURCES
    .github/workflows/test.yml
    .github/workflows/release.yml
)

set(CPACK_PACKAGE_CONTACT "Charles Flèche <charles.fleche@free.fr>")
set(CPACK_PACKAGE_VENDOR "charlesfleche")
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_PACKAGE_ICON ${CMAKE_SOURCE_DIR}/lpd8editor.svg)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE.md)

set(CPACK_GENERATOR "DEB;TGZ")

set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_SECTION sound)

include(CPack)
