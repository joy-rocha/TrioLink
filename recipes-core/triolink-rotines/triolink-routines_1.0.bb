SUMMARY = "Rotinas de inicializacao do TrioLink"
DESCRIPTION = "Comando triolink-routine para registrar programas que iniciam em todo boot, e o init script que os executa"
LICENSE = "CLOSED"

SRC_URI = " \
    file://triolink-routines \
    file://triolink-routine \
"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "triolink-routines"
INITSCRIPT_PARAMS = "start 95 2 3 4 5 . stop 05 0 1 6 ."

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/triolink-routines ${D}${sysconfdir}/init.d/triolink-routines

    install -d ${D}${sysconfdir}/triolink/routines.d

    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/triolink-routine ${D}${bindir}/triolink-routine
}

FILES:${PN} += "${sysconfdir}/triolink"
