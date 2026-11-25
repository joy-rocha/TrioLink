# Troca o getty do tty1 pela sessao TrioLink (wizard de primeiro boot + terminal).
# Os getty de console serial continuam como estao (acesso de emergencia).
#
# No systemd o "inittab" nao existe: o login do tty1 vem do getty@tty1.service.
# Aqui ele e removido do getty.target e mascarado (link para /dev/null); quem
# assume o tty1 e o triolink-session.service, instalado pela recipe
# triolink-terminal (ver recipes-triolink/triolink-terminal).
# O mascaramento tambem impede o logind de subir um getty no tty1 sozinho
# (autovt@tty1), que derrubaria a sessao TrioLink.
#
# Obs.: o triolink-terminal precisa estar na imagem (IMAGE_INSTALL), senao o
# tty1 fica sem login. Ele ja esta no triolink-image.bb.

do_install:append() {
    # remove qualquer ativacao do tty1 gerada pela recipe original
    rm -f ${D}${sysconfdir}/systemd/system/getty.target.wants/getty@tty1.service

    install -d ${D}${sysconfdir}/systemd/system
    ln -sf /dev/null ${D}${sysconfdir}/systemd/system/getty@tty1.service
}

FILES:${PN} += "${sysconfdir}/systemd/system/getty@tty1.service"
