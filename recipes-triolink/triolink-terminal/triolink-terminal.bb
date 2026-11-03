SUMMARY = "TRIOLINK custom terminal interface"
DESCRIPTION = "Terminal com logo ASCII persistente, iniciado no tty1 pelo inittab (via triolink-session)"
LICENSE = "CLOSED"

SRC_URI = " \
    file://triolink-terminal.c \
    file://bill-triolink.txt \
    file://triolink-session \
"

S = "${WORKDIR}"

DEPENDS += "ncurses libvterm"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} \
        ${S}/triolink-terminal.c \
        -o triolink-terminal \
        -lncurses -lvterm -lutil
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 triolink-terminal ${D}${bindir}/triolink-terminal
    install -m 0755 ${WORKDIR}/triolink-session ${D}${bindir}/triolink-session

    install -d ${D}${datadir}/triolink
    install -m 0644 ${WORKDIR}/bill-triolink.txt \
        ${D}${datadir}/triolink/bill-triolink.txt
}

FILES:${PN} += "${datadir}/triolink"

RDEPENDS:${PN} += "ncurses libvterm triolink-firstboot"