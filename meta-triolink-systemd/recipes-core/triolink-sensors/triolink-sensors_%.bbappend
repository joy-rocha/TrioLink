# Servico systemd do triolink-sensors.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://triolink-sensors.service"

inherit systemd

SYSTEMD_SERVICE:${PN} = "triolink-sensors.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/triolink-sensors.service ${D}${systemd_system_unitdir}/triolink-sensors.service
}
