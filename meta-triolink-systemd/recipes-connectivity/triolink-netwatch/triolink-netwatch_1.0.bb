SUMMARY = "Vigia a internet e reconecta o Wi-Fi quando cai (systemd)"
DESCRIPTION = "Servico que testa a conexao a cada poucos segundos e, se ela cair, tenta reconectar em etapas (reativar o perfil, religar o radio Wi-Fi, reiniciar o NetworkManager e, opcionalmente, reiniciar a placa)"
LICENSE = "CLOSED"

SRC_URI = " \
    file://triolink-netwatch \
    file://triolink-netwatch.service \
    file://triolink-netwatch.default \
"

S = "${WORKDIR}"

inherit systemd

SYSTEMD_SERVICE:${PN} = "triolink-netwatch.service"
SYSTEMD_AUTO_ENABLE = "enable"

RDEPENDS:${PN} = "networkmanager networkmanager-nmcli iputils-ping"

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/triolink-netwatch ${D}${sbindir}/triolink-netwatch

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/triolink-netwatch.service \
        ${D}${systemd_system_unitdir}/triolink-netwatch.service

    install -d ${D}${sysconfdir}/default
    install -m 0644 ${WORKDIR}/triolink-netwatch.default \
        ${D}${sysconfdir}/default/triolink-netwatch
}

CONFFILES:${PN} = "${sysconfdir}/default/triolink-netwatch"
