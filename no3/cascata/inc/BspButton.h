/*
 * @file    BspButton.h
 * @brief   Leitura dos botoes de SELECAO e ENTER com debounce.
 *          Gera eventos de clique que sao consumidos pela aplicacao.
 */

#ifndef BSP_BUTTON_H
#define BSP_BUTTON_H

#include <stdbool.h>

/// Periodo recomendado de chamada do BspButton_Poll [ms]
#define dBSP_BUTTON_POLL_PERIOD_MS      10

/// Eventos gerados pelos botoes
typedef enum
{
    eBSP_BUTTON_EVENT_NONE = 0,
    eBSP_BUTTON_EVENT_SELECT,
    eBSP_BUTTON_EVENT_ENTER
} bspButtonEvent_t;

/*
 * @brief   Reinicia o estado interno dos botoes e limpa a fila de eventos.
 *          O BspGpio_Init() deve ter sido chamado antes.
 * @param   void
 * @retval  true  - Inicializacao concluida
 * @retval  false - Falha ao ler o estado inicial dos botoes
 */
bool BspButton_Init(void);

/*
 * @brief   Amostra os botoes, aplica o debounce e enfileira os cliques.
 *          Deve ser chamada periodicamente a cada dBSP_BUTTON_POLL_PERIOD_MS.
 * @param   void
 * @retval  void
 */
void BspButton_Poll(void);

/*
 * @brief   Retira o evento mais antigo da fila.
 * @param   void
 * @retval  Evento do botao, ou eBSP_BUTTON_EVENT_NONE se a fila estiver vazia
 */
bspButtonEvent_t BspButton_GetEvent(void);

#endif /* BSP_BUTTON_H */