# Servico systemd do triolink-routines.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://triolink-routines.service"

inherit systemd

SYSTEMD_SERVICE:${PN} = "triolink-routines.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/triolink-routines.service ${D}${systemd_system_unitdir}/triolink-routines.service
}
