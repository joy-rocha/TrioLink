/**************************************
* @file Buttons.h
* @addtogroup Buttons
* @brief Driver de leitura de botoes com debounce e fila de eventos.
* @{
***************************************/

#ifndef _BUTTONS_H_
#define _BUTTONS_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include <stdbool.h>
#include <stdint.h>

/**************************************
* CONFIGURACOES
***************************************/

/**************************************
* DEFINES PUBLICOS
***************************************/
/// Pino BCM do botao de navegacao (preto - pino fisico 31)
#define dBUTTONS_PIN_NAV   6
/// Pino BCM do botao de confirmacao (vermelho - pino fisico 26)
#define dBUTTONS_PIN_ENTER 7

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Eventos dos botoes
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
* @brief Inicializa a leitura dos botoes e a thread de debounce.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Buttons_Init(void);

/**************************************
* @brief Retorna um evento pendente da fila de botoes.
* @param Nenhum.
* @retval Evento do botao (eBTN_EVENT_NONE se a fila estiver vazia).
***************************************/
btnEvent_t Buttons_Poll(void);

/**************************************
* @brief Finaliza a thread dos botoes e libera a interface GPIO.
* @param Nenhum.
* @retval Nenhum.
***************************************/
void Buttons_Close(void);

#endif /* _BUTTONS_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */