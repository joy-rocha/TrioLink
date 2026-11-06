/**************************************
* @file Ui.h
* @addtogroup Ui
* @brief Interface de utilizador e gestao de telas no display.
* @{
***************************************/

#ifndef _UI_H_
#define _UI_H_

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

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Telas disponiveis no sistema
typedef enum
{
    eSCREEN_HOME = 0,
    eSCREEN_BMP,
    eSCREEN_MPU,
    eSCREEN_OFF
} screen_t;

/// Acoes executadas pela interface
typedef enum
{
    eUI_ACTION_NONE = 0,
    eUI_ACTION_SHUTDOWN
} uiAction_t;

/// Estrutura com os dados processados dos sensores
typedef struct
{
    bool online;
    double value1;
    double value2;
    double value3;
    char status[16];
} sensorData_t;

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
/**************************************
/** @brief Renderiza a tela atual no framebuffer.
* @param screen: identificador da tela ativa.
* @param selectedIndex: indice do item selecionado na tela.
* @param bmpData: ponteiro para os dados do sensor BMP.
* @param mpuData: ponteiro para os dados do sensor MPU.
* @retval Nenhum.
***************************************/
void Ui_Render(screen_t screen, int selectedIndex, const sensorData_t *bmpData, const sensorData_t *mpuData);

/**************************************
/** @brief Avanca a selecao para o proximo item da tela atual.
* @param screen: identificador da tela ativa.
* @param selectedIndex: ponteiro para o indice selecionado.
* @retval Nenhum.
***************************************/
void Ui_Next(screen_t screen, int *selectedIndex);

/**************************************
/** @brief Processa a acao de confirmacao no item selecionado.
* @param screen: identificador da tela atual.
* @param selectedIndex: ponteiro para o indice selecionado.
* @param action: ponteiro para armazenar a acao resultante.
* @retval Nova tela a ser exibida.
***************************************/
screen_t Ui_Enter(screen_t screen, int *selectedIndex, uiAction_t *action);

#endif /* _UI_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */