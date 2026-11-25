SUMMARY = "Rotinas de inicializacao do TrioLink"
DESCRIPTION = "Comando triolink-routine para registrar programas que iniciam em todo boot, e o script que os executa. Quem o chama no boot (init script ou servico systemd) vem do triolink-routines_%.bbappend da layer do init."
LICENSE = "CLOSED"

SRC_URI = " \
    file://triolink-routines \
    file://triolink-routine \
"

S = "${WORKDIR}"

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/triolink-routines ${D}${sbindir}/triolink-routines

    install -d ${D}${sysconfdir}/triolink/routines.d

    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/triolink-routine ${D}${bindir}/triolink-routine
}

FILES:${PN} += "${sysconfdir}/triolink"
