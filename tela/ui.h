#ifndef UI_H
#define UI_H
#include <stdbool.h>

typedef enum { SCR_HOME, SCR_BMP, SCR_MPU, SCR_OFF } Screen;
typedef enum { ACT_NONE, ACT_SHUTDOWN } UiAction;

typedef struct {
    bool   online;
    double v1, v2, v3;      /* BMP: pressao, temperatura, altitude | MPU: direcao, velocidade */
    char   estado[16];      /* "normal" | "anormal" */
} SensorData;

/* sel = item selecionado na tela atual:
   HOME: 0 = BMP280, 1 = MPU6050, 2 = DESLIGAR
   OFF : 0 = SIM,    1 = NAO
   BMP/MPU: 0 = VOLTAR */
void   ui_render(Screen s, int sel, const SensorData *bmp, const SensorData *mpu);
void   ui_next(Screen s, int *sel);                          /* botão preto: próximo item */
Screen ui_enter(Screen s, int *sel, UiAction *act);          /* botão vermelho: confirma */

#endif
