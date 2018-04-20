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
