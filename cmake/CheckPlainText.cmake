# Fails if the sources use the static QMessageBox functions. Run by the
# 'plain_text_messages' test (tests/CMakeLists.txt):
#
#   cmake -DSOURCE_DIR=<src> -P CheckPlainText.cmake
#
# Those functions show their text as Qt::AutoText: Qt guesses whether it is
# HTML, and our messages hold paths and names from outside
# (doc/SECURITY.md, finding 6). No unit test can see such a call in a code
# path that needs a real dialog, so the sources themselves are checked. Use
# messagebox.h instead. QMessageBox::about() is allowed: 'About' is rich
# text on purpose, and its values are escaped.

file(GLOB_RECURSE _sources "${SOURCE_DIR}/*.cpp" "${SOURCE_DIR}/*.h")

set(_failures "")
foreach(_file IN LISTS _sources)
    file(STRINGS "${_file}" _matches
        REGEX "QMessageBox::(warning|information|critical|question)[ ]*\\(")
    foreach(_match IN LISTS _matches)
        list(APPEND _failures "${_file}: ${_match}")
    endforeach()
endforeach()

if (_failures)
    list(JOIN _failures "\n  " _list)
    message(FATAL_ERROR "Static QMessageBox calls guess HTML, use messagebox.h:\n  ${_list}")
endif()
list(LENGTH _sources _count)
if (_count EQUAL 0)
    message(FATAL_ERROR "No sources in ${SOURCE_DIR}: nothing was checked")
endif()
message(STATUS "${_count} files checked: no static QMessageBox call")
