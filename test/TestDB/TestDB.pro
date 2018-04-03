QT += testlib sql
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

INCLUDEPATH += $$PWD/../../src

SOURCES += \
    tst_db.cpp \
    $$PWD/../../src/db.cpp \
    $$PWD/../../src/lpd8_sysex.cpp \
    $$PWD/../../src/utils.cpp \

HEADERS += \
    $$PWD/../../src/db.h \
    $$PWD/../../src/utils.h
