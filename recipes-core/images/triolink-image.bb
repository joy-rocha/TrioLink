SUMMARY = "Imagem TrioLink"
LICENSE = "MIT"

require recipes-core/images/core-image-base.bb

#instala as dependencias aqui
IMAGE_INSTALL:append = " mosquitto"
IMAGE_INSTALL:append = " i2c-tools"
IMAGE_INSTALL:append = " cmake"
IMAGE_INSTALL:append = " lgpio"
IMAGE_INSTALL:append = " freetype"
IMAGE_INSTALL:append = " mosquitto-clients"
IMAGE_INSTALL:append = " dtc"
IMAGE_INSTALL:append = " mosquitto-dev"
#json
IMAGE_INSTALL:append = " cjson-dev"
#seria o build essentials que caio pediu
IMAGE_INSTALL:append = " packagegroup-core-buildessential"
IMAGE_INSTALL:append = " nano"
#pacote de redes para diagnóstico
IMAGE_INSTALL:append = " iputils-ping curl"

#gerenciamento de rede e conexão wi-fi
#IMAGE_INSTALL:append = " connman connman-client wpa-supplicant iw"
# Servidor SSH
IMAGE_FEATURES:append = " ssh-server-openssh"

#adicionando senha e ssh bootavel
IMAGE_INSTALL:append = " triolink-firstboot"

#adicionando a interface de terminal
IMAGE_INSTALL:append = " triolink-terminal"

#adicionado o sysinit para funcionar
IMAGE_INSTALL:append = " sysvinit"
IMAGE_INSTALL:append = " initscripts"

#colocando o teclado em português
IMAGE_INSTALL:append = " kbd kbd-keymaps"

#colocando .local no rasp
IMAGE_INSTALL:append = " avahi-daemon"

#adicionando para mostrar o ip 
IMAGE_INSTALL:append = " iproute2 net-tools"


#adicionando para editar dentro da rasp para acessar o wifi
IMAGE_INSTALL:append = " networkmanager networkmanager-nmtui networkmanager-nmcli wpa-supplicant iw dbus"

#adicionando conexão wifi do assert automaticamente
IMAGE_INSTALL:append = " triolink-wifi"

#adiciona o sudo
IMAGE_INSTALL:append = " sudo"

#adicionando I2C automatico
IMAGE_INSTALL:append = " triolink-sensors"