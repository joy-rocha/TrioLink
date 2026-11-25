SUMMARY = "Virtual terminal emulation library"
DESCRIPTION = "Library for terminal emulation"
HOMEPAGE = "https://github.com/neovim/libvterm"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=be5681ffe0dc58ccc9756bc6260fe0cd"

SRC_URI = "git://github.com/neovim/libvterm.git;protocol=https;nobranch=1"
SRCREV = "9d6d2112335080312ef8c36667fa717ded4f7daf"

S = "${WORKDIR}/git"
PV = "0.3.3"

DEPENDS += "libtool-cross"

EXTRA_OEMAKE = " \
    PREFIX=${prefix} \
    LIBDIR=${libdir} \
    INCDIR=${includedir} \
"

do_compile() {
    oe_runmake \
        CC="${CC}" \
        LIBTOOL="${STAGING_BINDIR_CROSS}/libtool" \
        CFLAGS="${CFLAGS}" \
        LDFLAGS="${LDFLAGS}"
}

do_install() {
    oe_runmake install \
        DESTDIR="${D}" \
        PREFIX="${prefix}" \
        LIBDIR="${libdir}" \
        INCDIR="${includedir}"
}

FILES:${PN} += "${libdir}/libvterm.so.*"
FILES:${PN}-dev += "${libdir}/libvterm.so ${includedir}/vterm.h ${includedir}/vterm"