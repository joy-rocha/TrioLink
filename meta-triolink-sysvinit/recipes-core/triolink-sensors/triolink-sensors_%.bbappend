# Init script (sysvinit) do triolink-sensors: /etc/init.d/triolink-sensors e um
# link relativo para o script instalado em ${sbindir} (ele aceita start/stop/restart).
inherit update-rc.d

INITSCRIPT_NAME = "triolink-sensors"
INITSCRIPT_PARAMS = "start 30 2 3 4 5 . stop 70 0 1 6 ."

do_install:append() {
    install -d ${D}${sysconfdir}/init.d
    ln -sf ../..${sbindir}/triolink-sensors ${D}${sysconfdir}/init.d/triolink-sensors
}
