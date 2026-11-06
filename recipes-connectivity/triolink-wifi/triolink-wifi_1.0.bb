SUMMARY = "Rede Wi-Fi pre-configurada para o NetworkManager"
LICENSE = "CLOSED"

SRC_URI = "file://triolink-wifi.nmconnection.in"

S = "${WORKDIR}"

# definidos no local.conf (nunca no layer)
TRIOLINK_WIFI_SSID ?= ""
TRIOLINK_WIFI_PSK ?= ""

RDEPENDS:${PN} = "networkmanager"
ALLOW_EMPTY:${PN} = "1"

do_install() {
    if [ -z "${TRIOLINK_WIFI_SSID}" ]; then
        bbwarn "TRIOLINK_WIFI_SSID nao definido: nenhuma rede Wi-Fi pre-configurada"
        return 0
    fi

    install -d -m 0755 ${D}${sysconfdir}/NetworkManager/system-connections
    install -m 0600 ${WORKDIR}/triolink-wifi.nmconnection.in \
        ${D}${sysconfdir}/NetworkManager/system-connections/triolink-wifi.nmconnection

    sed -i \
        -e "s|@SSID@|${TRIOLINK_WIFI_SSID}|g" \
        -e "s|@PSK@|${TRIOLINK_WIFI_PSK}|g" \
        ${D}${sysconfdir}/NetworkManager/system-connections/triolink-wifi.nmconnection
}

FILES:${PN} = "${sysconfdir}/NetworkManager/system-connections"