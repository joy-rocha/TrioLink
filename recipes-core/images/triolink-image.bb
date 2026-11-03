EXTRA_IMAGE_FEATURES ?= ""

USER_CLASSES ?= "buildstats"

PATCHRESOLVE = "noop"

BB_DISKMON_DIRS ??= "\
    STOPTASKS,${TMPDIR},1G,100K \
    STOPTASKS,${DL_DIR},1G,100K \
    STOPTASKS,${SSTATE_DIR},1G,100K \
    STOPTASKS,/tmp,100M,100K \
    HALT,${TMPDIR},100M,1K \
    HALT,${DL_DIR},100M,1K \
    HALT,${SSTATE_DIR},100M,1K \
    HALT,/tmp,10M,1K"

PACKAGECONFIG:append:pn-qemu-system-native = " sdl"

CONF_VERSION = "2"

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
IMAGE_INSTALL:append = " connman connman-client wpa-supplicant iw"
# Servidor SSH
IMAGE_FEATURES:append = " ssh-server-openssh"

#adicionando senha e ssh bootavel
IMAGE_INSTALL:append = " triolink-firstboot"

#adicionando a interface de terminal
IMAGE_INSTALL:append = " triolink-terminal"

#adicionado o sysinit para funcionar
IMAGE_INSTALL:append = " sysvinit"
IMAGE_INSTALL:append = " initscripts"

# Criar usuario e definir senha
INHERIT += "extrausers"


#é necessário para fazer o bitbake gerar na image
#tanto a conexão da chave SSH como a password
EXTRA_USERS_PARAMS = " \
    useradd -m -s /bin/sh triolink; \
    usermod -p '${TRIOLINK_PASSWD}' triolink; \
"


