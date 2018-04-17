TEMPLATE = subdirs

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /usr/bin
!isEmpty(target.path): INSTALLS += target

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
