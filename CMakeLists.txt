cmake_minimum_required(VERSION 3.10)

find_package(Git REQUIRED)
execute_process(
    COMMAND
        ${GIT_EXECUTABLE} describe --tags --long --dirty --always
    WORKING_DIRECTORY
        ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE
        GIT_DESCRIBE_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

string(REGEX MATCH [0-9]+\.[0-9]+\.[0-9]+
    _app_version
    ${GIT_DESCRIBE_VERSION}
)
message(STATUS "Git describe version: ${GIT_DESCRIBE_VERSION}")
message(STATUS "Project version: ${_app_version}")

project(lpd8editor
    VERSION ${_app_version}
    HOMEPAGE_URL https://github.com/charlesfleche/lpd8editor
    DESCRIPTION "A linux editor for the Akai LPD8"
    LANGUAGES CXX
)

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
    .github/workflows/actions.yml
)
