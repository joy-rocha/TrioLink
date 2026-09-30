/* Monitor de sensores (BMP280 + MPU6050) no display TFT 2.4" com 2 botões - Raspberry Pi 5
   Preto = navegar (troca a seleção) | Vermelho = enter (confirma) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "cJSON.h"
#include "Decoders.h"
#include "lcd.h"
#include "gfx.h"
#include "ui.h"
#include "buttons.h"

#define FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSansMono-Bold.ttf"

static volatile sig_atomic_t running = 1;
static void on_sig(int s) { (void)s; running = 0; }

static long ms_now(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1000L + t.tv_nsec / 1000000L;
}

/* ---- Fonte dos dados brutos ----
   Por enquanto são os JSONs simulados do seu main.c original.
   Troque o conteúdo destas duas funções pela leitura real dos sensores. */
static const char *obter_json_bmp(void) {
    return "{\"temperature\": 26.0, \"pressure\": 101300.0, \"altitude\": 550.0}";
}
static const char *obter_json_mpu(void) {
    return "{\"accel\": {\"x\": 10.0, \"y\": 5.0, \"z\": 12.0}}";
}

static double num(const cJSON *j, const char *k) {
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(j, k);
    return cJSON_IsNumber(it) ? it->valuedouble : 0.0;
}

static void copia_estado(SensorData *d, const cJSON *j) {
    const cJSON *e = cJSON_GetObjectItemCaseSensitive(j, "estado");
    snprintf(d->estado, sizeof d->estado, "%s", cJSON_IsString(e) ? e->valuestring : "--");
}

static void atualizar_bmp(SensorData *d) {
    memset(d, 0, sizeof *d);
    cJSON *j = processar_dados_bmp(obter_json_bmp());
    if (!j) { strcpy(d->estado, "--"); return; }
    d->online = true;
    d->v1 = num(j, "pressao");
    d->v2 = num(j, "temperatura");
    d->v3 = num(j, "altitude");
    copia_estado(d, j);
    cJSON_Delete(j);
}

static void atualizar_mpu(SensorData *d) {
    memset(d, 0, sizeof *d);
    cJSON *j = processar_dados_mpu(obter_json_mpu());
    if (!j) { strcpy(d->estado, "--"); return; }
    d->online = true;
    d->v1 = num(j, "direcao");
    d->v2 = num(j, "velocidade");
    copia_estado(d, j);
    cJSON_Delete(j);
}

static void desligar_sistema(void) {
    gfx_clear(rgb(18, 18, 20));
    gfx_text(LCD_W / 2, LCD_H / 2, 20, "DESLIGANDO...", rgb(235, 235, 240), ALIGN_CENTER);
    lcd_flush(fb);
    /* precisa de permissão: rode como root ou libere no sudoers (NOPASSWD) */
    int r = system("sudo /sbin/shutdown -h now");
    (void)r;
    running = 0;
}

/* Teste de fiação: ./monitor --btn-debug */
static int btn_debug(void) {
    printf("Aperte os botoes (Ctrl+C sai)\n");
    while (running) {
        BtnEvent e = btn_poll();
        if (e == BTN_NAV)        printf("PRETO    -> navegar\n");
        else if (e == BTN_ENTER) printf("VERMELHO -> enter\n");
        fflush(stdout);
        usleep(5000);
    }
    return 0;
}

int main(int argc, char **argv) {
    signal(SIGINT, on_sig);
    signal(SIGTERM, on_sig);

    if (gfx_init(FONT_PATH) < 0) return 1;
    if (lcd_init() < 0) return 1;
    if (btn_init() < 0) { lcd_close(); return 1; }
    if (argc > 1 && strcmp(argv[1], "--btn-debug") == 0) {
        btn_debug();
        btn_close();
        lcd_close();
        return 0;
    }

    SensorData bmp = {0}, mpu = {0};
    Screen scr = SCR_HOME;
    int sel = 0;
    bool dirty = true;
    long last_update = -10000;

    while (running) {
        long now = ms_now();

        /* 1) atualiza sensores a cada 1 s; só redesenha se algum valor mudou */
        if (now - last_update >= 1000) {
            SensorData nb, nm;
            atualizar_bmp(&nb);
            atualizar_mpu(&nm);
            if (memcmp(&nb, &bmp, sizeof bmp) != 0 || memcmp(&nm, &mpu, sizeof mpu) != 0) {
                bmp = nb;
                mpu = nm;
                dirty = true;
            }
            last_update = now;
        }

        /* 2) botões: esvazia a fila de toques acumulados */
        BtnEvent ev;
        while ((ev = btn_poll()) != BTN_NONE) {
            if (ev == BTN_NAV) {
                ui_next(scr, &sel);
                dirty = true;
            } else if (ev == BTN_ENTER) {
                UiAction act;
                scr = ui_enter(scr, &sel, &act);
                dirty = true;
                if (act == ACT_SHUTDOWN) { desligar_sistema(); break; }
            }
        }

        /* 3) redesenha só quando algo mudou */
        if (dirty && running) {
            ui_render(scr, sel, &bmp, &mpu);
            lcd_flush(fb);
            dirty = false;
        }
        usleep(10000);
    }

    btn_close();
    lcd_close();
    return 0;
}
