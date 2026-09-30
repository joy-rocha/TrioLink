#ifndef BUTTONS_H
#define BUTTONS_H

/* Numeração BCM (GPIOxx), NÃO o número do pino físico.
   Cada botão liga entre o GPIO e o GND (pino físico 6). Pull-up interno é ativado no código.

   BCM 6 = pino físico 31   (preto  - navegar)
   BCM 7 = pino físico 26   (vermelho - enter)

   ATENÇÃO: o pino físico 32 é o BCM 12, que o LCD já usa como D0. Por isso o preto foi para o 31. */
#define PIN_BTN_NAV    6
#define PIN_BTN_ENTER  7

typedef enum { BTN_NONE = 0, BTN_NAV, BTN_ENTER } BtnEvent;

int      btn_init(void);    /* 0 = ok, -1 = erro */
BtnEvent btn_poll(void);    /* retorna 1 evento pendente por chamada (BTN_NONE se não há) */
void     btn_close(void);

#endif
