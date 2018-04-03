QT += testlib
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../../src

SOURCES += \
    $$PWD/../../src/lpd8_sysex.cpp \
    $$PWD/../../src/utils.cpp \
    tst_utils.cpp

HEADERS += \
    $$PWD/../../src/lpd8_sysex.h \
    $$PWD/../../src/utils.h
