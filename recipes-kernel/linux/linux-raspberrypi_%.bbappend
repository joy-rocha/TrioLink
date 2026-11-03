# ele usa todos os arquivos que estão dentro do files
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

# arquivos usados pela recipe
SRC_URI += " \
    file://triolink-bmp280.cfg \
    file://triolink-mpu6500.cfg \
    file://triolink-bmp280-overlay.dts \
    file://triolink-mpu6500-overlay.dts \
"

# fragmentos de configuração do kernel
KCONFIG_FRAGMENTS += " \
    triolink-bmp280.cfg \
    triolink-mpu6500.cfg \
"

do_configure:append() {
    install -m 0644 ${WORKDIR}/triolink-bmp280-overlay.dts \
        ${S}/arch/arm64/boot/dts/overlays/

    install -m 0644 ${WORKDIR}/triolink-mpu6500-overlay.dts \
        ${S}/arch/arm64/boot/dts/overlays/
}

# garante inicialização no boot
RPI_KERNEL_DEVICETREE_OVERLAYS:append = " overlays/triolink-bmp280.dtbo"
RPI_KERNEL_DEVICETREE_OVERLAYS:append = " overlays/triolink-mpu6500.dtbo"