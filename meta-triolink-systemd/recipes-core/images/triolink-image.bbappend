# Parte da imagem TrioLink que depende do init: systemd.

#adicionado o systemd para funcionar
IMAGE_INSTALL:append = " systemd"
IMAGE_INSTALL:append = " systemd-compat-units"

#hora de Brasilia com NTP (o systemd-timesyncd ja vem no pacote systemd; triolink-time poe os servidores ntp.br)
IMAGE_INSTALL:append = " triolink-time"

#reconecta o Wi-Fi sozinho quando a internet cair
IMAGE_INSTALL:append = " triolink-netwatch"
