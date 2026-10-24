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
include(cmake/Fuzzing.cmake)
include(cmake/Format.cmake)
include(cmake/ClangTidy.cmake)
include(cmake/Cppcheck.cmake)

find_package(ALSA REQUIRED)
# Qt 6.8 is the version in Debian trixie, the oldest target we build and test.
find_package(Qt6 6.8 REQUIRED COMPONENTS Sql Widgets)

# CTest defines the BUILD_TESTING option (ON by default). Packagers can
# configure with -DBUILD_TESTING=OFF to skip building the tests.
include(cmake/Memcheck.cmake)
include(CTest)

add_subdirectory(src)
add_subdirectory(data)
if (BUILD_TESTING)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
    add_subdirectory(fuzz)
endif()

# Lists the CI files, so IDEs show them in the project tree.
add_custom_target(github
  SOURCES
    .github/dependabot.yml
    .github/workflows/quality.yml
    .github/workflows/release.yml
    .github/workflows/test.yml
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
# Qt loads these as plugins at run time, so dpkg-shlibdeps can't find them
# in the binary: the SQLite driver for the one-time migration from
# db.sqlite3 (migration.cpp; without it, a user upgrading with an old
# database gets a refusal to start, "Driver not loaded"), and the SVG image
# and icon plugins for the window icon. CPack adds what dpkg-shlibdeps finds.
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt6sql6-sqlite, qt6-svg-plugins")

# A package built on one distribution links against that distribution's
# libraries, so the release builds one per distribution, and names them
# apart with Debian's convention: lpd8editor_<version>-1~debian13_amd64.deb.
set(LPD8EDITOR_PACKAGE_DISTRO "" CACHE STRING
    "Distribution the packages are built for, added to their names (debian13, ubuntu26.04...)")
if (LPD8EDITOR_PACKAGE_DISTRO)
    set(CPACK_DEBIAN_PACKAGE_RELEASE "1~${LPD8EDITOR_PACKAGE_DISTRO}")
    set(CPACK_PACKAGE_FILE_NAME
        "${PROJECT_NAME}-${PROJECT_VERSION}-${LPD8EDITOR_PACKAGE_DISTRO}-${CMAKE_SYSTEM_PROCESSOR}")
endif()

include(CPack)
