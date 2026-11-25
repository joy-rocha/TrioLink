SUMMARY = "Registra os sensores I2C do TrioLink (BMP280 e MPU6500) no boot"
DESCRIPTION = "Script que carrega os drivers IIO e cria os dispositivos no barramento I2C 1. Quem o executa no boot (init script ou servico systemd) vem do triolink-sensors_%.bbappend da layer do init."
LICENSE = "CLOSED"

SRC_URI = "file://triolink-sensors"

S = "${WORKDIR}"

RDEPENDS:${PN} = "kmod"

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/triolink-sensors ${D}${sbindir}/triolink-sensors
}
