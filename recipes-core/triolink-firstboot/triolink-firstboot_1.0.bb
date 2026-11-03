SUMMARY = "Configuracao inicial do TrioLink no primeiro boot"
DESCRIPTION = "Wizard que solicita hostname, usuario e senha no primeiro boot (executado pelo triolink-session no tty1)"
LICENSE = "CLOSED"

SRC_URI = "file://triolink-firstboot"

S = "${WORKDIR}"

# useradd, usermod, userdel e chpasswd vem do shadow
RDEPENDS:${PN} = "shadow"

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/triolink-firstboot ${D}${sbindir}/triolink-firstboot
}