/* Leitura de 2 push buttons (pull-up interno, ativo em nível baixo) com debounce.
   Uma thread lê os botões continuamente e guarda os toques numa fila, assim nenhum
   aperto se perde enquanto o display está sendo redesenhado (lcd_flush é lento). */
#include "buttons.h"
#include <lgpio.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define GPIOCHIP     0
#define DEBOUNCE_MS  30
#define POLL_US      2000
#define QSIZE        16

typedef struct {
    int      pin;
    BtnEvent ev;
    int      stable;   /* último nível estável (1 = solto, 0 = apertado) */
    int      raw;      /* última leitura crua */
    long     t;        /* instante da última mudança crua */
} Btn;

static Btn btns[2] = {
    { PIN_BTN_NAV,   BTN_NAV,   1, 1, 0 },
    { PIN_BTN_ENTER, BTN_ENTER, 1, 1, 0 },
};

static int gh = -1;
static pthread_t th;
static volatile int run = 0;

static pthread_mutex_t mu = PTHREAD_MUTEX_INITIALIZER;
static BtnEvent q[QSIZE];
static int qh = 0, qt = 0;

static long ms_now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
}

static void push(BtnEvent e) {
    pthread_mutex_lock(&mu);
    int nt = (qt + 1) % QSIZE;
    if (nt != qh) { q[qt] = e; qt = nt; }
    pthread_mutex_unlock(&mu);
}

BtnEvent btn_poll(void) {
    BtnEvent e = BTN_NONE;
    pthread_mutex_lock(&mu);
    if (qh != qt) { e = q[qh]; qh = (qh + 1) % QSIZE; }
    pthread_mutex_unlock(&mu);
    return e;
}

static void *reader(void *arg) {
    (void)arg;
    while (run) {
        long now = ms_now();
        for (int i = 0; i < 2; i++) {
            Btn *b = &btns[i];
            int v = lgGpioRead(gh, b->pin);
            if (v < 0) continue;
            if (v != b->raw) {
                b->raw = v;
                b->t = now;
            } else if (v != b->stable && now - b->t >= DEBOUNCE_MS) {
                b->stable = v;
                if (v == 0) push(b->ev);     /* evento só no instante em que aperta */
            }
        }
        usleep(POLL_US);
    }
    return NULL;
}

int btn_init(void) {
    gh = lgGpiochipOpen(GPIOCHIP);
    if (gh < 0) { fprintf(stderr, "botoes: lgGpiochipOpen falhou (%d)\n", gh); return -1; }

    for (int i = 0; i < 2; i++) {
        int r = lgGpioClaimInput(gh, LG_SET_PULL_UP, btns[i].pin);
        if (r < 0) {
            fprintf(stderr, "botoes: GPIO %d indisponivel (%d). Esta em uso pelo LCD?\n", btns[i].pin, r);
            lgGpiochipClose(gh);
            gh = -1;
            return -1;
        }
    }

    run = 1;
    if (pthread_create(&th, NULL, reader, NULL) != 0) {
        run = 0;
        lgGpiochipClose(gh);
        gh = -1;
        return -1;
    }
    return 0;
}

void btn_close(void) {
    if (run) { run = 0; pthread_join(th, NULL); }
    if (gh >= 0) lgGpiochipClose(gh);
    gh = -1;
}
