SUMMARY = "Registra os sensores I2C do TrioLink (BMP280 e MPU6500) no boot"
DESCRIPTION = "Init script que carrega os drivers IIO e cria os dispositivos no barramento I2C 1"
LICENSE = "CLOSED"

SRC_URI = "file://triolink-sensors"

S = "${WORKDIR}"

inherit update-rc.d

INITSCRIPT_NAME = "triolink-sensors"
INITSCRIPT_PARAMS = "start 30 2 3 4 5 . stop 70 0 1 6 ."

RDEPENDS:${PN} = "kmod"

do_install() {
    install -d ${D}${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/triolink-sensors ${D}${sysconfdir}/init.d/triolink-sensors
}
