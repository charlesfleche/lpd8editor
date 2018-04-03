include(../../common.pri)
include(../common.pri)

SOURCES += \
    tst_utils.cpp \
    $$PWD/../../src/lpd8_sysex.cpp \
    $$PWD/../../src/utils.cpp

HEADERS += \
    $$PWD/../../src/lpd8_sysex.h \
    $$PWD/../../src/utils.h
