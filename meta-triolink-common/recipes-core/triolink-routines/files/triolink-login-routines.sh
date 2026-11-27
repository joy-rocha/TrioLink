# Rotinas de LOGIN do TrioLink (instalado em /etc/profile.d/).
#
# Inicia, em segundo plano, os programas ligados em /etc/triolink/login.d
# DEPOIS que alguem faz login digitando usuario e senha (tty1 ou ssh). Nada
# roda no boot: apos reiniciar ou voltar a energia o programa so sobe quando o
# usuario entrar.
#
# Regras:
#  - so conta login em terminal (com tty); comandos via ssh sem tty sao ignorados
#  - o destino do link pode ser absoluto (/home/jpyce/x) ou comecar com "~/",
#    que vale como a pasta pessoal de quem logou (serve para qualquer usuario)
#  - so dispara quando quem entrou e o DONO do programa (um login do root nao
#    inicia o que e do jpyce)
#  - se o programa ja estiver rodando, nao inicia outro; se tiver terminado ou
#    caido, o proximo login inicia de novo
#  - o programa roda com o usuario que logou, separado do terminal: continua
#    rodando depois do logout
#  - a saida fica em ~/.triolink/NOME.log

_triolink_login_routines() {
    _tl_dir="${TRIOLINK_LOGIN_DIR:-/etc/triolink/login.d}"
    [ -d "$_tl_dir" ] || return 0
    [ -t 0 ] || return 0
    [ -n "$HOME" ] && [ -d "$HOME" ] || return 0

    _tl_me=$(id -un 2>/dev/null) || return 0
    _tl_run="/tmp/triolink-login-$(id -u)"
    _tl_logd="$HOME/.triolink"

    for _tl_link in "$_tl_dir"/*; do
        [ -L "$_tl_link" ] || continue
        _tl_name="${_tl_link##*/}"
        # TRIOLINK_ONLY=NOME inicia so essa rotina (usado por "triolink-routine start")
        [ -z "$TRIOLINK_ONLY" ] || [ "$TRIOLINK_ONLY" = "$_tl_name" ] || continue
        _tl_raw=$(readlink "$_tl_link" 2>/dev/null)
        # "~/..." vale como a pasta pessoal de quem logou, entao o mesmo registro
        # funciona em qualquer placa, seja qual for o nome do usuario
        case "$_tl_raw" in
            "~/"*) _tl_target="$HOME/${_tl_raw#"~/"}" ;;
            *)     _tl_target=$(readlink -f "$_tl_link" 2>/dev/null) ;;
        esac

        if [ ! -f "$_tl_target" ] || [ ! -x "$_tl_target" ]; then
            # avisa so se o programa era do usuario que acabou de entrar
            case "$_tl_raw" in
                "$HOME"/*|"~/"*)
                    echo "triolink: rotina '$_tl_name': '$_tl_target' nao existe ou nao e executavel" >&2
                    ;;
            esac
            continue
        fi

        [ "$(stat -c %U "$_tl_target" 2>/dev/null)" = "$_tl_me" ] || continue

        mkdir -p "$_tl_run" "$_tl_logd" 2>/dev/null || continue
        _tl_pidf="$_tl_run/$_tl_name.pid"

        # ja esta rodando? (confere o pid e se o processo e mesmo o programa)
        if [ -f "$_tl_pidf" ]; then
            _tl_pid=$(cat "$_tl_pidf" 2>/dev/null)
            if [ -n "$_tl_pid" ] && kill -0 "$_tl_pid" 2>/dev/null; then
                case "$(tr '\0' ' ' < "/proc/$_tl_pid/cmdline" 2>/dev/null)" in
                    *"$_tl_target"*) continue ;;
                esac
            fi
        fi

        if command -v setsid >/dev/null 2>&1; then _tl_det=setsid; else _tl_det=nohup; fi

        # espera opcional pela rede: arquivo "login.d/.NOME.wait" com os segundos
        _tl_wait=0
        if [ -f "$_tl_dir/.$_tl_name.wait" ]; then
            _tl_wait=$(cat "$_tl_dir/.$_tl_name.wait" 2>/dev/null)
            case "$_tl_wait" in ""|*[!0-9]*) _tl_wait=0 ;; esac
        fi

        # o sh grava o proprio pid e vira o programa (exec), entao o pid vale.
        # Se pediram para esperar a rede, a espera acontece aqui dentro, em segundo
        # plano: o login nao trava. Passado o limite, o programa inicia mesmo assim.
        # HUP ignorado so durante o disparo: o processo filho herda isso e nao
        # morre se o terminal fechar logo em seguida (como o nohup faz).
        trap '' HUP
        $_tl_det sh -c '
echo $$ > "$3"
if [ "$4" -gt 0 ]; then
    echo "triolink: aguardando a rede (ate ${4}s)..."
    n=0
    while [ "$n" -lt "$4" ]; do
        if command -v nmcli >/dev/null 2>&1; then
            case "$(nmcli -t -f STATE general 2>/dev/null)" in
                connected|"connected (site only)") break ;;
            esac
        elif ip route show default 2>/dev/null | grep -q .; then
            break
        fi
        sleep 2
        n=$((n + 2))
    done
    if [ "$n" -lt "$4" ]; then
        echo "triolink: rede pronta apos ${n}s"
    else
        echo "triolink: rede nao ficou pronta em ${4}s, iniciando assim mesmo"
    fi
fi
cd "$1" && exec "$2"' sh \
            "${_tl_target%/*}" "$_tl_target" "$_tl_pidf" "$_tl_wait" \
            </dev/null >>"$_tl_logd/$_tl_name.log" 2>&1 &
        trap - HUP
        echo "triolink: rotina '$_tl_name' iniciada (log: $_tl_logd/$_tl_name.log)"
    done
}

_triolink_login_routines
unset -f _triolink_login_routines 2>/dev/null
unset _tl_dir _tl_me _tl_run _tl_logd _tl_link _tl_name _tl_raw _tl_target _tl_pidf _tl_pid _tl_det _tl_wait