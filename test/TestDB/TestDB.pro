include(../../common.pri)
include(../common.pri)

SOURCES += \
    tst_db.cpp \
    $$PWD/../../src/db.cpp \
    $$PWD/../../src/lpd8_sysex.cpp \
    $$PWD/../../src/utils.cpp \

HEADERS += \
    $$PWD/../../src/db.h \
    $$PWD/../../src/utils.h
