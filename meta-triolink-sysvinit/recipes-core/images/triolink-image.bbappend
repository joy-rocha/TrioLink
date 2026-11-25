# Parte da imagem TrioLink que depende do init: sysvinit.

#adicionado o sysinit para funcionar
IMAGE_INSTALL:append = " sysvinit"
IMAGE_INSTALL:append = " initscripts"

#hora de Brasilia com NTP (chrony; no sysvinit nao existe systemd-timesyncd)
IMAGE_INSTALL:append = " chrony"
