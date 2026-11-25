# Integra o triolink-terminal ao systemd: o triolink-session.service assume o
# tty1 no lugar do getty@tty1 (que e mascarado em recipes-core/systemd).
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += "file://triolink-session.service"

inherit systemd

SYSTEMD_SERVICE:${PN} = "triolink-session.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/triolink-session.service \
        ${D}${systemd_system_unitdir}/triolink-session.service
}
