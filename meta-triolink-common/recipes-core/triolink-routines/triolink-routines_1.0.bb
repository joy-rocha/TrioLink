SUMMARY = "Rotinas de inicializacao do TrioLink"
DESCRIPTION = "Comando triolink-routine para registrar programas que iniciam em todo boot, e o script que os executa. Tambem inicia programas de LOGIN (so depois que o usuario digita usuario e senha). Quem chama as rotinas de boot (init script ou servico systemd) vem do triolink-routines_%.bbappend da layer do init."
LICENSE = "CLOSED"

SRC_URI = " \
    file://triolink-routines \
    file://triolink-routine \
    file://triolink-login-routines.sh \
"

S = "${WORKDIR}"

# --- Rotinas de LOGIN: iniciam so DEPOIS que alguem entra com usuario e senha ---
# Formato "NOME:CAMINHO", separadas por espaco. Apos reiniciar ou voltar a
# energia nada roda sozinho: o programa sobe quando o dono dele faz login (tty1
# ou ssh), em segundo plano, com o proprio usuario e a saida em ~/.triolink/NOME.log.
# Detalhes em files/triolink-login-routines.sh (instalado em /etc/profile.d/).
#
# O programa NAO precisa existir no build (o cascata e compilado na propria placa).
# "~/" vale como a pasta pessoal de quem logou (/home/<usuario>), entao o mesmo
# build serve para qualquer placa, seja qual for o nome do usuario criado no
# wizard de primeiro boot. O programa precisa ser do usuario que loga.
# Para mudar ou acrescentar, sobrescreva no local.conf:
#   TRIOLINK_LOGIN_ROUTINES = "cascata:~/gloriaa/cascata/build/cascata outra:~/outra"
TRIOLINK_LOGIN_ROUTINES ?= "cascata:~/gloriaa/cascata/build/cascata"

# Nomes (da lista acima) que devem ESPERAR A REDE conectar antes de iniciar. Logo
# depois do boot o Wi-Fi ainda esta conectando; sem essa espera, programas que
# usam rede (ex.: MQTT) podem falhar. A espera e em segundo plano, nao trava o
# login, e passado TRIOLINK_LOGIN_WAIT_SECONDS o programa inicia mesmo assim.
TRIOLINK_LOGIN_WAIT_NETWORK ?= "cascata"
TRIOLINK_LOGIN_WAIT_SECONDS ?= "90"

# --- Rotinas de BOOT: iniciam no boot, antes de qualquer login (como root ou dono) ---
# Mesmo formato. Vazio por padrao.
TRIOLINK_ROUTINES ?= ""

do_install() {
    install -d ${D}${sbindir}
    install -m 0755 ${WORKDIR}/triolink-routines ${D}${sbindir}/triolink-routines

    install -d ${D}${sysconfdir}/triolink/routines.d
    install -d ${D}${sysconfdir}/triolink/login.d

    # rotinas de boot (link em routines.d -> programa)
    for entry in ${TRIOLINK_ROUTINES}; do
        name="${entry%%:*}"
        target="${entry#*:}"
        if [ "$name" = "$entry" ] || [ -z "$name" ] || [ -z "$target" ]; then
            bbfatal "TRIOLINK_ROUTINES invalido: '$entry' (use NOME:CAMINHO)"
        fi
        ln -sf "$target" ${D}${sysconfdir}/triolink/routines.d/"$name"
    done

    # rotinas de login (link em login.d -> programa)
    for entry in ${TRIOLINK_LOGIN_ROUTINES}; do
        name="${entry%%:*}"
        target="${entry#*:}"
        if [ "$name" = "$entry" ] || [ -z "$name" ] || [ -z "$target" ]; then
            bbfatal "TRIOLINK_LOGIN_ROUTINES invalido: '$entry' (use NOME:CAMINHO)"
        fi
        ln -sf "$target" ${D}${sysconfdir}/triolink/login.d/"$name"
    done

    # rotinas de login que esperam a rede (arquivo oculto login.d/.NOME.wait)
    for name in ${TRIOLINK_LOGIN_WAIT_NETWORK}; do
        echo "${TRIOLINK_LOGIN_WAIT_SECONDS}" > ${D}${sysconfdir}/triolink/login.d/".$name.wait"
    done

    # lido pelo shell de login (/etc/profile) em todo login
    install -d ${D}${sysconfdir}/profile.d
    install -m 0644 ${WORKDIR}/triolink-login-routines.sh \
        ${D}${sysconfdir}/profile.d/triolink-login-routines.sh

    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/triolink-routine ${D}${bindir}/triolink-routine
}

FILES:${PN} += "${sysconfdir}/triolink ${sysconfdir}/profile.d"