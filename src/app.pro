include(../common.pri)

TARGET = lpd8-editor

LIBS += -L. -llpd8editor

SOURCES += main.cpp

RESOURCES += \
    sql.qrc

