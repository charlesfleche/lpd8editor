QT += testlib sql
QT -= gui

CONFIG += qt console warn_on depend_includepath testcase
CONFIG -= app_bundle

TEMPLATE = app

RESOURCES += $$PWD/../src/sql.qrc

INCLUDEPATH += $$PWD/../src $$PWD/include
LIBS += -L../../src -llpd8editor
