/* Telas (layout 320x240) - navegação por 2 botões físicos (navegar / enter) */
#include "ui.h"
#include "gfx.h"
#include <stdio.h>
#include <string.h>

typedef struct { int x, y, w, h; } Rect;

static const Rect R_CARD_BMP = { 12,  44, 144, 110 };
static const Rect R_CARD_MPU = { 164, 44, 144, 110 };
static const Rect R_POWER    = { 12, 166, 296,  62 };
static const Rect R_BACK     = { 204,  4, 108,  36 };
static const Rect R_YES      = { 28, 150, 124,  56 };
static const Rect R_NO       = { 168, 150, 124,  56 };

/* Cores */
#define C_BG    rgb(18, 18, 20)
#define C_CARD  rgb(38, 39, 50)
#define C_TEXT  rgb(235, 235, 240)
#define C_GREEN rgb(25, 232, 138)
#define C_GLOW  rgb(20, 90, 60)
#define C_RED   rgb(255, 51, 85)

/* Quantos itens selecionáveis cada tela tem */
static int n_items(Screen s) {
    switch (s) {
    case SCR_HOME: return 3;
    case SCR_OFF:  return 2;
    default:       return 1;
    }
}

/* Item que já vem selecionado ao entrar em uma tela */
static int start_sel(Screen from, Screen to) {
    if (to == SCR_HOME) return from == SCR_MPU ? 1 : from == SCR_OFF ? 2 : 0;
    if (to == SCR_OFF)  return 1;      /* começa em NÃO, mais seguro */
    return 0;
}

/* Cartão arredondado; se selecionado, ganha uma borda verde */
static void frame(const Rect *r, int rad, bool sel) {
    if (sel) gfx_fill_rrect(r->x - 3, r->y - 3, r->w + 6, r->h + 6, rad + 3, C_GREEN);
    gfx_fill_rrect(r->x, r->y, r->w, r->h, rad, C_CARD);
}

static void header(const char *title, bool back) {
    gfx_text(12, 22, 18, title, C_TEXT, ALIGN_LEFT);
    if (back) {
        frame(&R_BACK, 10, true);        /* VOLTAR é o único item, sempre selecionado */
        gfx_text(R_BACK.x + R_BACK.w / 2, R_BACK.y + R_BACK.h / 2, 16, "< VOLTAR", C_TEXT, ALIGN_CENTER);
    }
}

static void led(int cx, int cy, bool online) {
    if (online) gfx_fill_circle(cx, cy, 9, C_GLOW);
    gfx_fill_circle(cx, cy, 6, online ? C_GREEN : C_RED);
}

static void icon_power(int cx, int cy) {
    gfx_ring(cx, cy, 13, 13, 3, C_GREEN);
    gfx_fill_rect(cx - 6, cy - 18, 12, 10, C_CARD);     /* abre a lacuna no topo */
    gfx_fill_rrect(cx - 2, cy - 17, 4, 17, 2, C_GREEN); /* traço vertical */
}

static void icon_chip(const Rect *box) {
    int cx = box->x + box->w / 2, cy = box->y + box->h / 2;
    for (int i = -1; i <= 1; i += 1) {                   /* pinos */
        gfx_fill_rect(cx - 30, cy + i * 12 - 1, 14, 3, C_GREEN);
        gfx_fill_rect(cx + 16, cy + i * 12 - 1, 14, 3, C_GREEN);
        gfx_fill_rect(cx + i * 12 - 1, cy - 30, 3, 14, C_GREEN);
        gfx_fill_rect(cx + i * 12 - 1, cy + 16, 3, 14, C_GREEN);
    }
    gfx_fill_rrect(cx - 18, cy - 18, 36, 36, 5, C_GREEN);
    gfx_fill_rrect(cx - 16, cy - 16, 32, 32, 4, rgb(30, 31, 38));
    gfx_fill_circle(cx - 9, cy - 9, 2, C_GREEN);
}

static void icon_atom(const Rect *box) {
    int cx = box->x + box->w / 2, cy = box->y + box->h / 2;
    gfx_ring(cx, cy, 30, 12, 3, C_GREEN);
    gfx_ring(cx, cy, 12, 30, 3, C_GREEN);
    gfx_fill_circle(cx, cy, 4, C_RED);
}

static void value_box(int y, const char *label, const char *value) {
    gfx_text(104, y + 17, 14, label, C_TEXT, ALIGN_LEFT);
    gfx_fill_rrect(208, y, 104, 34, 8, C_CARD);
    gfx_text(260, y + 17, 18, value, C_TEXT, ALIGN_CENTER);
}

static void estado_row(const SensorData *d) {
    gfx_text(12, 212, 18, "ESTADO:", C_TEXT, ALIGN_LEFT);
    if (!d->online)
        gfx_text(110, 212, 18, "OFFLINE", C_RED, ALIGN_LEFT);
    else if (strcmp(d->estado, "anormal") == 0)
        gfx_text(110, 212, 18, "ANORMAL", C_RED, ALIGN_LEFT);
    else
        gfx_text(110, 212, 18, "NORMAL", C_GREEN, ALIGN_LEFT);
}

void ui_render(Screen s, int sel, const SensorData *bmp, const SensorData *mpu) {
    char buf[32];
    gfx_clear(C_BG);

    switch (s) {
    case SCR_HOME:
        gfx_text(LCD_W / 2, 22, 18, "SISTEMA DE MONITORAMENTO", C_TEXT, ALIGN_CENTER);
        frame(&R_CARD_BMP, 12, sel == 0);
        frame(&R_CARD_MPU, 12, sel == 1);
        gfx_text(R_CARD_BMP.x + R_CARD_BMP.w / 2, R_CARD_BMP.y + 58, 20, "BMP280", C_TEXT, ALIGN_CENTER);
        gfx_text(R_CARD_MPU.x + R_CARD_MPU.w / 2, R_CARD_MPU.y + 58, 20, "MPU6050", C_TEXT, ALIGN_CENTER);
        led(R_CARD_BMP.x + 20, R_CARD_BMP.y + 20, bmp->online);
        led(R_CARD_MPU.x + 20, R_CARD_MPU.y + 20, mpu->online);
        frame(&R_POWER, 12, sel == 2);
        icon_power(R_POWER.x + 40, R_POWER.y + 31);
        gfx_text(R_POWER.x + 70, R_POWER.y + 31, 18, "DESLIGAR SISTEMA", C_TEXT, ALIGN_LEFT);
        gfx_text(R_POWER.x + R_POWER.w - 22, R_POWER.y + 31, 20, ">", C_TEXT, ALIGN_CENTER);
        break;

    case SCR_BMP: {
        header("BMP280", true);
        Rect ic = { 10, 52, 84, 128 };
        gfx_fill_rrect(ic.x, ic.y, ic.w, ic.h, 12, C_CARD);
        icon_chip(&ic);
        snprintf(buf, sizeof buf, "%.0f hPa", bmp->v1);
        value_box(52, "PRESSÃO:", buf);
        snprintf(buf, sizeof buf, "%.0f° C", bmp->v2);
        value_box(98, "TEMPERATURA:", buf);
        snprintf(buf, sizeof buf, "%.0f m", bmp->v3);
        value_box(144, "ALTITUDE:", buf);
        estado_row(bmp);
        break;
    }

    case SCR_MPU: {
        header("MPU6050", true);
        Rect ic = { 10, 52, 84, 128 };
        gfx_fill_rrect(ic.x, ic.y, ic.w, ic.h, 12, C_CARD);
        icon_atom(&ic);
        snprintf(buf, sizeof buf, "%.0f deg", mpu->v1);
        value_box(72, "DIREÇÃO:", buf);
        snprintf(buf, sizeof buf, "%.1f m/s", mpu->v2);
        value_box(126, "VELOCIDADE:", buf);
        estado_row(mpu);
        break;
    }

    case SCR_OFF:
        header("DESLIGAR SISTEMA", false);
        gfx_text(LCD_W / 2, 92, 18, "TEM CERTEZA QUE DESEJA", C_TEXT, ALIGN_CENTER);
        gfx_text(LCD_W / 2, 118, 18, "DESLIGAR O SISTEMA?", C_TEXT, ALIGN_CENTER);
        frame(&R_YES, 12, sel == 0);
        frame(&R_NO,  12, sel == 1);
        gfx_text(R_YES.x + R_YES.w / 2, R_YES.y + R_YES.h / 2, 22, "SIM", C_TEXT, ALIGN_CENTER);
        gfx_text(R_NO.x + R_NO.w / 2, R_NO.y + R_NO.h / 2, 22, "NÃO", C_TEXT, ALIGN_CENTER);
        break;
    }
}

/* Botão PRETO: passa para o próximo item da tela */
void ui_next(Screen s, int *sel) {
    *sel = (*sel + 1) % n_items(s);
}

/* Botão VERMELHO: confirma o item selecionado */
Screen ui_enter(Screen s, int *sel, UiAction *act) {
    Screen n = s;
    *act = ACT_NONE;

    switch (s) {
    case SCR_HOME:
        n = (*sel == 0) ? SCR_BMP : (*sel == 1) ? SCR_MPU : SCR_OFF;
        break;
    case SCR_BMP:
    case SCR_MPU:
        n = SCR_HOME;                                   /* único item: VOLTAR */
        break;
    case SCR_OFF:
        if (*sel == 0) { *act = ACT_SHUTDOWN; return s; }  /* SIM */
        n = SCR_HOME;                                   /* NÃO */
        break;
    }

    if (n != s) *sel = start_sel(s, n);
    return n;
}
