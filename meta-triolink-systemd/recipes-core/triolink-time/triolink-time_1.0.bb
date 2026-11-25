SUMMARY = "Hora de Brasilia (NTP) para o TrioLink"
DESCRIPTION = "Configura o systemd-timesyncd com servidores NTP brasileiros. O fuso America/Sao_Paulo (Brasilia-DF) vem do DEFAULT_TIMEZONE no triolink.conf."
LICENSE = "CLOSED"

SRC_URI = "file://10-triolink-time.conf"

S = "${WORKDIR}"

# no scarthgap o systemd-timesyncd ja vem dentro do pacote "systemd" (PACKAGECONFIG timesyncd)
RDEPENDS:${PN} = "systemd tzdata"

do_install() {
    install -d ${D}${sysconfdir}/systemd/timesyncd.conf.d
    install -m 0644 ${WORKDIR}/10-triolink-time.conf \
        ${D}${sysconfdir}/systemd/timesyncd.conf.d/10-triolink-time.conf
}
