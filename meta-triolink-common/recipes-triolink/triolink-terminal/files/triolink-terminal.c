#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <pty.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

#include <ncurses.h>
#include <vterm.h>

#define DATA_DIR "/usr/share/triolink"
#define MESSAGES_FILE DATA_DIR "/triolink-messages.txt"
#define BILL_FILE DATA_DIR "/bill-triolink.txt"

#define MAX_ART_LINES 100
#define MAX_LINE_LEN 512

static volatile sig_atomic_t resize_pending = 0;
static volatile sig_atomic_t child_pending = 0;

static int master_fd = -1;
static pid_t child_pid = -1;

static VTerm *vt = NULL;
static VTermScreen *screen = NULL;

static WINDOW *shell_win = NULL;
static WINDOW *art_win = NULL;

static int rows, cols;
static int shell_rows, shell_cols;
static int art_x, art_width;

static char *art_lines[MAX_ART_LINES];
static size_t art_count;

/* Area util da arte depois de aparar margens (ver prepare_art). */
static size_t art_first;   /* primeira linha com conteudo */
static size_t art_last;    /* uma depois da ultima linha com conteudo */
static size_t art_indent;  /* recuo comum a todas as linhas */
static size_t art_cols;    /* largura real da arte, sem recuo */

static char *messages[MAX_ART_LINES];
static size_t message_count;

static void handle_winch(int sig)
{
    (void)sig;
    resize_pending = 1;
}

static void handle_chld(int sig)
{
    (void)sig;
    child_pending = 1;
}

static size_t read_lines(const char *path, char **out, size_t max_lines)
{
    FILE *fp = fopen(path, "r");
    char *line = NULL;
    size_t cap = 0, count = 0;
    ssize_t len;

    if (!fp)
        return 0;

    while (count < max_lines &&
           (len = getline(&line, &cap, fp)) >= 0) {
        while (len > 0 &&
               (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = '\0';

        out[count] = strdup(line);
        if (!out[count])
            break;
        count++;
    }

    free(line);
    fclose(fp);
    return count;
}

static void free_lines(char **lines, size_t count)
{
    for (size_t i = 0; i < count; i++)
        free(lines[i]);
}

static bool line_is_blank(const char *s)
{
    return s[strspn(s, " \t")] == '\0';
}

/*
 * Tira as linhas em branco do comeco/fim do arquivo, os espacos no fim de
 * cada linha e o recuo comum a esquerda. Assim a arte ocupa so o espaco que
 * realmente precisa (o arquivo tem ~80 colunas e ~53 linhas, mas o desenho
 * em si e menor).
 */
static void prepare_art(void)
{
    size_t first = 0, last = art_count;
    size_t indent = (size_t)-1, width = 0;

    while (first < last && line_is_blank(art_lines[first]))
        first++;
    while (last > first && line_is_blank(art_lines[last - 1]))
        last--;

    for (size_t i = first; i < last; i++) {
        size_t len = strlen(art_lines[i]);

        while (len > 0 && (art_lines[i][len - 1] == ' ' ||
                           art_lines[i][len - 1] == '\t'))
            art_lines[i][--len] = '\0';

        if (len == 0)
            continue;

        size_t lead = strspn(art_lines[i], " ");
        if (lead < indent)
            indent = lead;
        if (len > width)
            width = len;
    }

    if (indent == (size_t)-1)
        indent = 0;

    art_first = first;
    art_last = last;
    art_indent = indent;
    art_cols = width > indent ? width - indent : 0;
}

static void draw_art(void)
{
    if (!art_win || art_width < 3 || rows < 3)
        return;

    werase(art_win);
    box(art_win, 0, 0);

    int inner_w = art_width - 2;   /* colunas uteis dentro da borda */

    /* Titulo */
    if (rows > 6) {
        wattron(art_win, A_BOLD);
        mvwaddnstr(art_win, 1, 2, "TRIOLINK", inner_w - 1);
        wattroff(art_win, A_BOLD);
    }

    /* Mensagens ficam coladas no rodape; a arte nunca passa por cima delas. */
    int msg_lines = 0;
    if (message_count > 0 && rows > 8)
        msg_lines = message_count > 2 ? 2 : (int)message_count;

    /* Area da arte: da linha 3 (abaixo do titulo) ate antes das mensagens. */
    int top = 3;
    int avail_h = rows - 1 - msg_lines - (msg_lines ? 1 : 0) - top;
    int art_h = (int)(art_last - art_first);

    if (avail_h > 0 && art_h > 0 && art_cols > 0) {
        /* Vertical: se couber, centraliza; se nao, corta igual em cima e embaixo. */
        int skip_y = 0, pad_y = 0;
        if (art_h > avail_h)
            skip_y = (art_h - avail_h) / 2;
        else
            pad_y = (avail_h - art_h) / 2;

        /* Horizontal: se couber, centraliza; se nao, corta igual dos dois lados. */
        int draw_w = (int)art_cols;
        int skip_x = 0, pad_x = 0;
        if (draw_w > inner_w) {
            skip_x = (draw_w - inner_w) / 2;
            draw_w = inner_w;
        } else {
            pad_x = (inner_w - draw_w) / 2;
        }

        for (int row = 0; row < avail_h && row < art_h - skip_y; row++) {
            const char *line = art_lines[art_first + (size_t)(skip_y + row)];
            size_t len = strlen(line);
            size_t start = art_indent + (size_t)skip_x;

            if (start >= len)
                continue;

            size_t n = len - start;
            if (n > (size_t)draw_w)
                n = (size_t)draw_w;

            mvwaddnstr(art_win, top + pad_y + row, 1 + pad_x,
                       line + start, (int)n);
        }
    }

    for (int i = 0; i < msg_lines; i++) {
        int len = (int)strlen(messages[i]);
        int x = 1;

        if (len < inner_w)
            x += (inner_w - len) / 2;

        mvwaddnstr(art_win, rows - 1 - msg_lines + i, x,
                   messages[i], inner_w);
    }

    wnoutrefresh(art_win);
}

/* ---- cores/atributos: traduz VTermColor para ncurses ---- */

static short pair_table[9][9];   /* [fg+1][bg+1] -> numero do par (0 = nao criado) */
static short next_pair = 1;

/* Converte uma cor do libvterm para 0..7 (ou -1 = padrao). */
static short map_color(const VTermColor *c, bool *bright)
{
    *bright = false;

    if (VTERM_COLOR_IS_DEFAULT_FG(c) || VTERM_COLOR_IS_DEFAULT_BG(c))
        return -1;

    if (VTERM_COLOR_IS_INDEXED(c)) {
        int i = c->indexed.idx;

        if (i < 8)
            return (short)i;
        if (i < 16) {
            *bright = true;
            return (short)(i - 8);
        }
        if (i >= 232)                       /* escala de cinza */
            return i < 244 ? 0 : 7;
        i -= 16;                            /* cubo 6x6x6 */
        {
            int r = (i / 36) > 2, g = ((i / 6) % 6) > 2, b = (i % 6) > 2;
            /* ncurses: 1=vermelho 2=verde 4=azul */
            return (short)((b ? 4 : 0) | (g ? 2 : 0) | (r ? 1 : 0));
        }
    }

    /* RGB */
    return (short)((c->rgb.blue > 127 ? 4 : 0) |
                   (c->rgb.green > 127 ? 2 : 0) |
                   (c->rgb.red > 127 ? 1 : 0));
}

/* ncurses numera cores como BGR (1=vermelho,4=azul), o libvterm/ANSI como
 * RGB (1=vermelho,4=azul tambem): os dois coincidem para 0..7. */

static short get_pair(short fg, short bg)
{
    if (!has_colors() || (fg < 0 && bg < 0))
        return 0;
    if (fg > 7 || bg > 7)
        return 0;

    short *slot = &pair_table[fg + 1][bg + 1];
    if (*slot == 0) {
        if (next_pair >= COLOR_PAIRS)
            return 0;
        init_pair(next_pair, fg, bg);
        *slot = next_pair++;
    }
    return *slot;
}

static void render_shell(void)
{
    if (!screen || !shell_win)
        return;

    werase(shell_win);

    for (int y = 0; y < shell_rows; y++) {
        for (int x = 0; x < shell_cols; x++) {
            VTermScreenCell cell;
            memset(&cell, 0, sizeof(cell));

            if (!vterm_screen_get_cell(screen, (VTermPos){y, x}, &cell))
                continue;

            uint32_t cp = cell.chars[0];

            /* segunda metade de um caractere largo */
            if (cp == (uint32_t)-1)
                continue;

            bool fg_bright, bg_bright;
            short fg = map_color(&cell.fg, &fg_bright);
            short bg = map_color(&cell.bg, &bg_bright);

            attr_t attr = A_NORMAL;
            if (cell.attrs.reverse)
                attr |= A_REVERSE;
            if (cell.attrs.bold || fg_bright)
                attr |= A_BOLD;
            if (cell.attrs.underline)
                attr |= A_UNDERLINE;

            /* celula vazia so precisa ser desenhada se tiver fundo/inverso */
            if (!cp) {
                if (!(attr & A_REVERSE) && bg < 0)
                    continue;
                cp = ' ';
            }

            wattrset(shell_win, attr | COLOR_PAIR(get_pair(fg, bg)));

            if (cp < 0x80) {
                mvwaddch(shell_win, y, x, (chtype)cp);
            } else {
                char utf8[5] = {0};

                if (cp < 0x800) {
                    utf8[0] = 0xC0 | (cp >> 6);
                    utf8[1] = 0x80 | (cp & 0x3F);
                } else if (cp < 0x10000) {
                    utf8[0] = 0xE0 | (cp >> 12);
                    utf8[1] = 0x80 | ((cp >> 6) & 0x3F);
                    utf8[2] = 0x80 | (cp & 0x3F);
                } else {
                    utf8[0] = 0xF0 | (cp >> 18);
                    utf8[1] = 0x80 | ((cp >> 12) & 0x3F);
                    utf8[2] = 0x80 | ((cp >> 6) & 0x3F);
                    utf8[3] = 0x80 | (cp & 0x3F);
                }

                mvwaddstr(shell_win, y, x, utf8);
            }

            if (cell.width == 2)
                x++;
        }
    }

    wattrset(shell_win, A_NORMAL);

    /* posiciona o cursor onde o programa (shell/nano) o deixou */
    {
        VTermPos cur;
        vterm_state_get_cursorpos(vterm_obtain_state(vt), &cur);
        if (cur.row >= 0 && cur.row < shell_rows &&
            cur.col >= 0 && cur.col < shell_cols)
            wmove(shell_win, cur.row, cur.col);
    }

    wnoutrefresh(shell_win);
}

static void resize_ui(void)
{
    endwin();
    refresh();
    clear();

    getmaxyx(stdscr, rows, cols);

    if (shell_win) {
        delwin(shell_win);
        shell_win = NULL;
    }

    if (art_win) {
        delwin(art_win);
        art_win = NULL;
    }

    /*
     * Painel da direita: tenta caber a arte inteira (+ 2 colunas de borda),
     * mas nunca passa de metade da tela para preservar largura do shell.
     */
    art_width = cols / 3;
    if (art_width < (int)art_cols + 2)
        art_width = (int)art_cols + 2;
    if (art_width < 24)
        art_width = 24;
    if (art_width > cols / 2)
        art_width = cols / 2;

    if (cols < 50)
        art_width = 0;

    art_x = cols - art_width;
    shell_cols = art_width ? art_x : cols;
    shell_rows = rows;

    if (shell_cols < 1)
        shell_cols = 1;
    if (shell_rows < 1)
        shell_rows = 1;

    shell_win = newwin(shell_rows, shell_cols, 0, 0);
    if (!shell_win) {
        endwin();
        fprintf(stderr, "triolink-terminal: nao foi possivel criar painel.\n");
        exit(1);
    }

    scrollok(shell_win, TRUE);
    keypad(shell_win, TRUE);

    if (art_width > 0) {
        art_win = newwin(rows, art_width, 0, art_x);
        if (art_win)
            draw_art();
    }

    if (vt) {
        vterm_set_size(vt, shell_rows, shell_cols);
        vterm_screen_flush_damage(screen);
    }

    if (master_fd >= 0) {
        struct winsize ws = {
            .ws_row = (unsigned short)shell_rows,
            .ws_col = (unsigned short)shell_cols
        };
        ioctl(master_fd, TIOCSWINSZ, &ws);
        if (child_pid > 0)
            kill(child_pid, SIGWINCH);
    }

    render_shell();
    wnoutrefresh(stdscr);
    doupdate();
}

static int launch_shell(void)
{
    struct winsize ws = {
        .ws_row = (unsigned short)shell_rows,
        .ws_col = (unsigned short)shell_cols
    };

    const char *shell = getenv("SHELL");
    if (!shell || !*shell)
        shell = "/bin/sh";

    child_pid = forkpty(&master_fd, NULL, NULL, &ws);

    if (child_pid < 0)
        return -1;

    if (child_pid == 0) {
        /* O libvterm emula um terminal estilo xterm, nao o console "linux". */
        setenv("TERM", "xterm", 1);
        execl(shell, shell, (char *)NULL);
        perror("triolink-terminal: execl");
        _exit(127);
    }

    int flags = fcntl(master_fd, F_GETFL);
    if (flags >= 0)
        (void)fcntl(master_fd, F_SETFL, flags | O_NONBLOCK);

    return 0;
}

static void send_key(int key)
{
    const char *seq = NULL;
    char c = 0;

    switch (key) {
    case KEY_UP:        seq = "\033[A"; break;
    case KEY_DOWN:      seq = "\033[B"; break;
    case KEY_RIGHT:     seq = "\033[C"; break;
    case KEY_LEFT:      seq = "\033[D"; break;
    case KEY_HOME:      seq = "\033[H"; break;
    case KEY_END:       seq = "\033[F"; break;
    case KEY_DC:        seq = "\033[3~"; break;
    case KEY_BACKSPACE: c = 127; break;
    case KEY_IC:        seq = "\033[2~"; break;
    case KEY_PPAGE:     seq = "\033[5~"; break;
    case KEY_NPAGE:     seq = "\033[6~"; break;
    case KEY_BTAB:      seq = "\033[Z"; break;
    /* Enter e CR (13), como um teclado real; o ICRNL do pty converte para
     * shells e o nano distingue de Ctrl+J (10). */
    case KEY_ENTER:     c = '\r'; break;
    default:
        if (key >= 0 && key <= 255) {
            c = (char)key;
            (void)write(master_fd, &c, 1);
        }
        return;
    }

    if (seq)
        (void)write(master_fd, seq, strlen(seq));
    else
        (void)write(master_fd, &c, 1);
}

int main(void)
{
    struct sigaction sa;
    bool running = true;
    int status = 0;

    setlocale(LC_ALL, "");

    art_count = read_lines(BILL_FILE, art_lines, MAX_ART_LINES);
    message_count = read_lines(MESSAGES_FILE, messages, MAX_ART_LINES);
    prepare_art();

    initscr();
    if (has_colors()) {
        start_color();
        use_default_colors();
    }
    raw();
    noecho();
    nonl();   /* Enter chega como '\r' (13) e nao como '\n' (10) */
    keypad(stdscr, TRUE);
    timeout(0);
    curs_set(1);

    memset(&sa, 0, sizeof(sa));
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handle_winch;
    sigaction(SIGWINCH, &sa, NULL);
    sa.sa_handler = handle_chld;
    sigaction(SIGCHLD, &sa, NULL);

    getmaxyx(stdscr, rows, cols);
    resize_ui();

    vt = vterm_new(shell_rows, shell_cols);
    if (!vt) {
        endwin();
        fprintf(stderr, "triolink-terminal: falha ao criar libvterm.\n");
        free_lines(art_lines, art_count);
        free_lines(messages, message_count);
        return 1;
    }

    vterm_set_utf8(vt, 1);
    screen = vterm_obtain_screen(vt);
    vterm_screen_reset(screen, 1);
    vterm_screen_flush_damage(screen);

    if (launch_shell() < 0) {
        endwin();
        fprintf(stderr, "triolink-terminal: forkpty falhou: %s\n",
                strerror(errno));
        vterm_free(vt);
        free_lines(art_lines, art_count);
        free_lines(messages, message_count);
        return 1;
    }

    while (running) {
        fd_set rfds;
        struct timeval tv = {.tv_sec = 0, .tv_usec = 20000};
        unsigned char buffer[4096];

        if (resize_pending) {
            resize_pending = 0;
            resize_ui();
        }

        FD_ZERO(&rfds);
        FD_SET(master_fd, &rfds);

        int rc = select(master_fd + 1, &rfds, NULL, NULL, &tv);

        if (rc > 0 && FD_ISSET(master_fd, &rfds)) {
            ssize_t n = read(master_fd, buffer, sizeof(buffer));

            if (n > 0) {
                vterm_input_write(vt, (const char *)buffer, (size_t)n);
                vterm_screen_flush_damage(screen);
                if (art_win)
                    draw_art();
                render_shell();   /* por ultimo: o cursor fica no shell */
                doupdate();
            } else if (n == 0 ||
                       (errno != EAGAIN && errno != EWOULDBLOCK &&
                        errno != EINTR)) {
                running = false;
            }
        }

        int key;
        while ((key = getch()) != ERR) {
            if (key == KEY_RESIZE) {
                resize_pending = 1;
            } else if (key == 3) {
                (void)write(master_fd, "\003", 1);
            } else if (key == 4) {
                (void)write(master_fd, "\004", 1);
            } else {
                send_key(key);
            }
        }

        if (child_pending) {
            child_pending = 0;
            pid_t result = waitpid(child_pid, &status, WNOHANG);
            if (result == child_pid)
                running = false;
        }
    }

    if (child_pid > 0) {
        pid_t result = waitpid(child_pid, &status, WNOHANG);
        if (result == 0) {
            kill(child_pid, SIGHUP);
            (void)waitpid(child_pid, &status, 0);
        }
    }

    if (master_fd >= 0)
        close(master_fd);

    if (shell_win)
        delwin(shell_win);
    if (art_win)
        delwin(art_win);

    endwin();

    vterm_free(vt);
    free_lines(art_lines, art_count);
    free_lines(messages, message_count);

    if (WIFEXITED(status))
        return WEXITSTATUS(status);

    return 0;
}