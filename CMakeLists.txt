cmake_minimum_required(VERSION 3.25)


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
	ERROR_QUIET
    )

    message(STATUS "Git describe version: ${LPD8EDITOR_QAPPLICATION_VERSION}")

    if (LPD8EDITOR_QAPPLICATION_VERSION)
        string(REGEX MATCH [0-9]+[.][0-9]+[.][0-9]+
            _parsed_git_describe_version
            ${LPD8EDITOR_QAPPLICATION_VERSION}
        )

        message(STATUS "Parsed git describe version: ${_parsed_git_describe_version}")

        if (_parsed_git_describe_version VERSION_GREATER _project_version)
	    message(STATUS "Found a valid version from git describe")
            set(_project_version ${_parsed_git_describe_version})
        endif()
    else()
        message(STATUS "Could not find a version from git describe")
    endif()
endif()

if (NOT LPD8EDITOR_QAPPLICATION_VERSION)
    message(STATUS "Trying to get a version from source folder name ${CMAKE_SOURCE_DIR}")
    string(REGEX MATCH [0-9]+[.][0-9]+[.][0-9]+$
           _parsed_source_folder_name_version
	   ${CMAKE_SOURCE_DIR}
    )
    if (_parsed_source_folder_name_version VERSION_GREATER _project_version)
        set(_project_version ${_parsed_source_folder_name_version})
    else()
        message(STATUS "Could not find a version from source folder name ${CMAKE_SOURCE_DIR}")
    endif()

    set(LPD8EDITOR_QAPPLICATION_VERSION ${_project_version})
endif()

project(lpd8editor
    VERSION ${_project_version}
    HOMEPAGE_URL https://github.com/charlesfleche/lpd8editor
    DESCRIPTION "A linux editor for the Akai LPD8"
    LANGUAGES CXX
)

message(STATUS "Project version: ${CMAKE_PROJECT_VERSION}")
message(STATUS "QApplication version: ${LPD8EDITOR_QAPPLICATION_VERSION}")

# The language standard is set per target (see src/CMakeLists.txt).
# Turn off GNU extensions so the code stays portable between GCC and Clang.
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)
include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)
include(cmake/Format.cmake)

find_package(ALSA REQUIRED)
# Qt 6.8 is the version in Debian trixie, the oldest target we build and test.
find_package(Qt6 6.8 REQUIRED COMPONENTS Sql Svg Widgets)

# CTest defines the BUILD_TESTING option (ON by default). Packagers can
# configure with -DBUILD_TESTING=OFF to skip building the tests.
include(cmake/Memcheck.cmake)
include(CTest)

add_subdirectory(src)
if (BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

add_custom_target(github
  SOURCES
    .github/workflows/test.yml
    .github/workflows/release.yml
)

set(CPACK_PACKAGE_DIRECTORY ${CMAKE_BINARY_DIR}/packages)
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
