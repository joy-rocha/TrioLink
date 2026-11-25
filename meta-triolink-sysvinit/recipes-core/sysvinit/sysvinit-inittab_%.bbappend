# Troca o getty do tty1 pela sessao TrioLink (wizard de primeiro boot + terminal).
# Os getty de console serial continuam como estao (acesso de emergencia).
#
# "getty -n -l <programa>" nao pede login e executa <programa> no tty1, ja como
# lider de sessao com terminal de controle. Se a imagem usar util-linux
# (agetty) em vez do getty do busybox, ajuste o caminho abaixo para
# ${base_sbindir}/agetty (as opcoes -n e -l tambem existem la).

do_install:append() {
    # remove qualquer linha do tty1 gerada pela recipe original
    sed -i '/^1:/d' ${D}${sysconfdir}/inittab

    echo "1:2345:respawn:${base_sbindir}/getty -n -l ${bindir}/triolink-session 38400 tty1" \
        >> ${D}${sysconfdir}/inittab
}

# garante que o programa chamado pelo getty exista na imagem
RDEPENDS:${PN} += "triolink-terminal"