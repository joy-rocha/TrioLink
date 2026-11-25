# Configuracao comum das distros TrioLink (systemd e sysvinit).
# Quem inclui este arquivo (triolink-systemd.conf / triolink-sysvinit.conf)
# define DISTRO, DISTRO_NAME e INIT_MANAGER.

# habilita o I2C no config.txt (meta-raspberrypi)
ENABLE_I2C = "1"

LICENSE_FLAGS_ACCEPTED += "synaptics-killswitch"

#desabilita aquele aviso chato que não tem energia suficiente
RPI_EXTRA_CONFIG:append = "\nusb_max_current_enable=1\n"

#trazendo o pacote do nmtui
PACKAGECONFIG:append:pn-networkmanager = " nmtui"

#adicionando a ativação automática do barramento I2C
RPI_EXTRA_CONFIG:append = "\ndtoverlay=triolink-bmp280\ndtoverlay=triolink-mpu6500\n"
KERNEL_MODULE_AUTOLOAD:append = " bmp280-i2c inv-mpu6050-i2c"

# fuso de Brasilia-DF (America/Sao_Paulo, UTC-3, sem horario de verao desde 2019)
DEFAULT_TIMEZONE = "America/Sao_Paulo"

# layout da imagem: /boot por LABEL com nofail (ver wic/triolink-sdimage.wks)
WKS_FILE = "triolink-sdimage.wks"