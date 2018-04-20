QT += sql svg widgets

INSTALL_PREFIX = $$(INSTALL_PREFIX)
isEmpty( INSTALL_PREFIX ) {
    INSTALL_PREFIX = $$[QT_INSTALL_PREFIX]
}
INSTALL_BIN = $${INSTALL_PREFIX}/bin

CONFIG += c++11 link_pkgconfig warn_on
QMAKE_CXXFLAGS += -Werror

PKGCONFIG += alsa
