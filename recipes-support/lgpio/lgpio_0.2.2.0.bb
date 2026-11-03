SUMMARY = "Linux GPIO library"
DESCRIPTION = "C library for controlling GPIOs on Linux SBCs"
HOMEPAGE = "https://github.com/joan2937/lg"
SECTION = "libs"

LICENSE = "Unlicense"
LIC_FILES_CHKSUM = "file://UNLICENCE;md5=61287f92700ec1bdf13bc86d8228cd13"

SRC_URI = "git://github.com/joan2937/lg.git;protocol=https;branch=master"

SRCREV = "b959a17d723360e85648316757b02dbea9902feb"

S = "${WORKDIR}/git"

EXTRA_OEMAKE = "CC='${CC}' AR='${AR}' RANLIB='${RANLIB}' STRIP='${STRIP}'"

INSANE_SKIP:${PN} += "already-stripped"

do_compile() {
    oe_runmake
}

do_install() {
   install -d ${D}${libdir}
   install -d ${D}${includedir}

   install -m 0755 ${S}/liblgpio.so.1 ${D}${libdir}/liblgpio.so.1
   ln -sf liblgpio.so.1 ${D}${libdir}/liblgpio.so

   install -m 0644 ${S}/lgpio.h ${D}${includedir}/lgpio.h
}

FILES:${PN} += "${libdir}/liblgpio.so.1"
FILES:${PN}-dev += "${includedir}/lgpio.h ${libdir}/liblgpio.so"
