/**************************************
* @file Buttons.h
* @addtogroup Buttons
* @brief Modulo de leitura e debounce de botoes via GPIO.
* @{
***************************************/

#ifndef _BUTTONS_H_
#define _BUTTONS_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include <stdbool.h>

/**************************************
* CONFIGURACOES
***************************************/

/**************************************
* DEFINES PUBLICOS
***************************************/
/// Pino BCM do botao de navegacao
#define dBUTTONS_PIN_BTN_NAV   6
/// Pino BCM do botao enter
#define dBUTTONS_PIN_BTN_ENTER 7

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Eventos gerados pelo acionamento dos botoes
typedef enum
{
    eBTN_EVENT_NONE = 0,
    eBTN_EVENT_NAV,
    eBTN_EVENT_ENTER
} btnEvent_t;

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
/**************************************
/** @brief Inicializa o driver de botoes e a thread de leitura.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Buttons_Init(void);

/**************************************
/** @brief Retorna um evento pendente da fila de botoes.
* @param Nenhum.
* @retval Evento do botao (eBTN_EVENT_NONE se nao houver evento).
***************************************/
btnEvent_t Buttons_Poll(void);

/**************************************
/** @brief Encerra a thread de leitura e libera os recursos GPIO.
* @param Nenhum.
* @retval Nenhum.
***************************************/
void Buttons_Close(void);

#endif /* _BUTTONS_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */