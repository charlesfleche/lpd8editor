include(common.pri)

TEMPLATE = subdirs

DISTFILES += \
    README.md \
    NOTES.md \
    LICENSE.md \
    doc/SYSEX.md

SUBDIRS += \
        src \
        test

test.depends += src

TRANSLATIONS = translations/*.ts

trans.files += translations/*.qm
trans.path = $${INSTALL_TRANSLATION}

INSTALLS += trans
