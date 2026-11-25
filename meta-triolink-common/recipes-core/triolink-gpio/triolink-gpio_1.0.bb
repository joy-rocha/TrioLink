SUMMARY = "Permissoes de GPIO, I2C e SPI para usuario sem root"
DESCRIPTION = "Cria os grupos gpio, i2c e spi e a regra udev que libera /dev/gpiochip*, /dev/i2c-* e /dev/spidev* para esses grupos (o wizard do primeiro boot coloca o usuario neles)"
LICENSE = "CLOSED"

SRC_URI = "file://99-triolink-gpio.rules"

S = "${WORKDIR}"

inherit useradd

USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM:${PN} = "-r gpio; -r i2c; -r spi"

do_install() {
    install -d ${D}${sysconfdir}/udev/rules.d
    install -m 0644 ${WORKDIR}/99-triolink-gpio.rules \
        ${D}${sysconfdir}/udev/rules.d/99-triolink-gpio.rules
}

FILES:${PN} += "${sysconfdir}/udev/rules.d/99-triolink-gpio.rules"
