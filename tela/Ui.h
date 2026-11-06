/**************************************
* @file Ui.h
* @addtogroup Ui
* @brief Interface do usuario, definicoes de telas e acoes de navegacao.
* @{
***************************************/

#ifndef _UI_H_
#define _UI_H_

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

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Identificadores das telas do sistema
typedef enum
{
    eUI_SCREEN_HOME = 0,
    eUI_SCREEN_BMP,
    eUI_SCREEN_MPU,
    eUI_SCREEN_OFF
} screen_t;

/// Acoes resultantes da selecao da UI
typedef enum
{
    eUI_ACTION_NONE = 0,
    eUI_ACTION_SHUTDOWN
} uiAction_t;

/// Estrutura com os dados processados do sensor
typedef struct
{
    bool isOnline;
    double val1;
    double val2;
    double val3;
    char statusStr[16];
} sensorData_t;

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
/**************************************
* @brief Desenha a tela ativa com base no estado e selecao atual.
* @param screen: tela a ser exibida.
* @param selectedItem: indice do item selecionado na tela.
* @param bmpDataPtr: ponteiro para os dados do sensor BMP280.
* @param mpuDataPtr: ponteiro para os dados do sensor MPU6050.
* @retval Nenhum.
***************************************/
void Ui_Render(screen_t screen, int selectedItem, const sensorData_t *bmpDataPtr, const sensorData_t *mpuDataPtr);

/**************************************
* @brief Avanca para o proximo item navegavel da tela (botao NAV).
* @param screen: tela atual.
* @param selectedItemPtr: ponteiro para o indice do item selecionado.
* @retval Nenhum.
***************************************/
void Ui_Next(screen_t screen, int *selectedItemPtr);

/**************************************
* @brief Confirma a selecao do item atual (botao ENTER).
* @param screen: tela atual.
* @param selectedItemPtr: ponteiro para o indice do item selecionado.
* @param actionPtr: ponteiro para armazenar a acao executada.
* @retval Proxima tela a ser carregada.
***************************************/
screen_t Ui_Enter(screen_t screen, int *selectedItemPtr, uiAction_t *actionPtr);

#endif /* _UI_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */