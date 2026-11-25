# Init script (sysvinit) do triolink-routines: /etc/init.d/triolink-routines e um
# link relativo para o script instalado em ${sbindir} (ele aceita start/stop/restart/status).
inherit update-rc.d

INITSCRIPT_NAME = "triolink-routines"
INITSCRIPT_PARAMS = "start 95 2 3 4 5 . stop 05 0 1 6 ."

do_install:append() {
    install -d ${D}${sysconfdir}/init.d
    ln -sf ../..${sbindir}/triolink-routines ${D}${sysconfdir}/init.d/triolink-routines
}
