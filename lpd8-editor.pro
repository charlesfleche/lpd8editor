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

TRANSLATIONS = \
    translations/lpd8-editor.fr.ts

trans.files += translations/lpd8-editor.fr.qm
trans.path = $${INSTALL_TRANSLATION}

INSTALLS += trans
