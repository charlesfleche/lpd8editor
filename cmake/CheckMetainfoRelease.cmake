# Run by the metainfo_release_version test (data/CMakeLists.txt):
#
#     cmake -DMETAINFO=<file> -DVERSION=<x.y.z> -P CheckMetainfoRelease.cmake
#
# Fails if the newest <release> of the AppStream metadata (the first one:
# AppStream lists them newest first) is not VERSION. On a release tag, this
# catches a release whose metainfo was not updated before tagging.
file(READ "${METAINFO}" _xml)
string(REGEX MATCH "<release[^>]* version=\"([^\"]+)\"" _match "${_xml}")
if (NOT _match)
    message(FATAL_ERROR "${METAINFO} has no <release version=\"...\"> entry")
endif()
if (NOT CMAKE_MATCH_1 STREQUAL VERSION)
    message(FATAL_ERROR
        "The newest <release> in ${METAINFO} is ${CMAKE_MATCH_1}, but this "
        "build is version ${VERSION}: add a <release version=\"${VERSION}\" "
        "date=\"YYYY-MM-DD\"/> entry before tagging.")
endif()
message(STATUS "The newest <release> is ${VERSION}")
