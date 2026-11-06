/**************************************
* @file Ui.c
* @addtogroup Ui
* @brief Layout e renderizacao de telas (320x240) para o monitor de sensores.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - Gfx
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 05/11/2026</b> \n Assert \n Primeira versao adaptada.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "Ui.h"
#include "Gfx.h"
#include <stdio.h>
#include <string.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Cor de fundo da interface
#define dUI_COLOR_BG    Gfx_Rgb(18, 18, 20)
/// Cor dos cartoes e caixas de dados
#define dUI_COLOR_CARD  Gfx_Rgb(38, 39, 50)
/// Cor principal dos textos
#define dUI_COLOR_TEXT  Gfx_Rgb(235, 235, 240)
/// Cor verde de status normal/destaque
#define dUI_COLOR_GREEN Gfx_Rgb(25, 232, 138)
/// Cor vermelha de status de alerta/offline
#define dUI_COLOR_RED   Gfx_Rgb(255, 51, 85)

/**************************************
* CONSTANTES
***************************************/

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Estrutura para retangulos de layout de tela
typedef struct
{
    int posX;
    int posY;
    int width;
    int height;
} rect_t;

/// Dimensoes dos elementos visuais estaticos
static const rect_t dUI_RECT_CARD_BMP = { 12, 44, 144, 110 };
static const rect_t dUI_RECT_CARD_MPU = { 164, 44, 144, 110 };
static const rect_t dUI_RECT_POWER    = { 12, 166, 296, 62 };
static const rect_t dUI_RECT_BACK     = { 204, 4, 108, 36 };
static const rect_t dUI_RECT_YES      = { 28, 150, 124, 56 };
static const rect_t dUI_RECT_NO       = { 168, 150, 124, 56 };

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static int Ui_GetItemCount(screen_t screen);
static int Ui_GetStartSelection(screen_t fromScreen, screen_t toScreen);
static void Ui_DrawFrame(const rect_t *rectPtr, int radius, bool isSelected);
static void Ui_DrawHeader(const char *titleStr, bool showBackButton);
static void Ui_DrawLed(int centerX, int centerY, bool isOnline);
static void Ui_DrawPowerIcon(int centerX, int centerY);
static void Ui_DrawChipIcon(const rect_t *boxPtr);
static void Ui_DrawAtomIcon(const rect_t *boxPtr);
static void Ui_DrawValueBox(int posY, const char *labelStr, const char *valueStr);
static void Ui_DrawStatusRow(const sensorData_t *dataPtr);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
* @brief Desenha a tela ativa com base no estado e selecao atual.
* @param screen: tela a ser exibida.
* @param selectedItem: indice do item selecionado na tela.
* @param bmpDataPtr: ponteiro para os dados do sensor BMP280.
* @param mpuDataPtr: ponteiro para os dados do sensor MPU6050.
* @retval Nenhum.
***************************************/
void Ui_Render(screen_t screen, int selectedItem, const sensorData_t *bmpDataPtr, const sensorData_t *mpuDataPtr)
{
    char textBuffer[32];
    Gfx_Clear(dUI_COLOR_BG);

    switch(screen)
    {
    case eUI_SCREEN_HOME:
        Gfx_Text(dLCD_W / 2, 22, 18, "SISTEMA DE MONITORAMENTO", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
        Ui_DrawFrame(&dUI_RECT_CARD_BMP, 12, (selectedItem == 0));
        Ui_DrawFrame(&dUI_RECT_CARD_MPU, 12, (selectedItem == 1));
        Gfx_Text(dUI_RECT_CARD_BMP.posX + (dUI_RECT_CARD_BMP.width / 2), dUI_RECT_CARD_BMP.posY + 58, 20, "BMP280", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
        Gfx_Text(dUI_RECT_CARD_MPU.posX + (dUI_RECT_CARD_MPU.width / 2), dUI_RECT_CARD_MPU.posY + 58, 20, "MPU6050", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
        Ui_DrawLed(dUI_RECT_CARD_BMP.posX + 20, dUI_RECT_CARD_BMP.posY + 20, bmpDataPtr->isOnline);
        Ui_DrawLed(dUI_RECT_CARD_MPU.posX + 20, dUI_RECT_CARD_MPU.posY + 20, mpuDataPtr->isOnline);
        Ui_DrawFrame(&dUI_RECT_POWER, 12, (selectedItem == 2));
        Ui_DrawPowerIcon(dUI_RECT_POWER.posX + 40, dUI_RECT_POWER.posY + 31);
        Gfx_Text(dUI_RECT_POWER.posX + 70, dUI_RECT_POWER.posY + 31, 18, "DESLIGAR SISTEMA", dUI_COLOR_TEXT, eGFX_ALIGN_LEFT);
        Gfx_Text(dUI_RECT_POWER.posX + dUI_RECT_POWER.width - 22, dUI_RECT_POWER.posY + 31, 20, ">", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
        break;

    case eUI_SCREEN_BMP:
        Ui_DrawHeader("BMP280", true);
        const rect_t iconBoxBmp = { 10, 52, 84, 128 };
        Gfx_FillRRect(iconBoxBmp.posX, iconBoxBmp.posY, iconBoxBmp.width, iconBoxBmp.height, 12, dUI_COLOR_CARD);
        Ui_DrawChipIcon(&iconBoxBmp);
        snprintf(textBuffer, sizeof(textBuffer), "%.0f hPa", bmpDataPtr->val1);
        Ui_DrawValueBox(52, "PRESSAO:", textBuffer);
        snprintf(textBuffer, sizeof(textBuffer), "%.0f C", bmpDataPtr->val2);
        Ui_DrawValueBox(98, "TEMPERATURA:", textBuffer);
        snprintf(textBuffer, sizeof(textBuffer), "%.0f m", bmpDataPtr->val3);
        Ui_DrawValueBox(144, "ALTITUDE:", textBuffer);
        Ui_DrawStatusRow(bmpDataPtr);
        break;

    case eUI_SCREEN_MPU:
        Ui_DrawHeader("MPU6050", true);
        const rect_t iconBoxMpu = { 10, 52, 84, 128 };
        Gfx_FillRRect(iconBoxMpu.posX, iconBoxMpu.posY, iconBoxMpu.width, iconBoxMpu.height, 12, dUI_COLOR_CARD);
        Ui_DrawAtomIcon(&iconBoxMpu);
        snprintf(textBuffer, sizeof(textBuffer), "%.0f deg", mpuDataPtr->val1);
        Ui_DrawValueBox(72, "DIRECAO:", textBuffer);
        snprintf(textBuffer, sizeof(textBuffer), "%.1f m/s", mpuDataPtr->val2);
        Ui_DrawValueBox(126, "VELOCIDADE:", textBuffer);
        Ui_DrawStatusRow(mpuDataPtr);
        break;

    case eUI_SCREEN_OFF:
        Ui_DrawHeader("DESLIGAR SISTEMA", false);
        Gfx_Text(dLCD_W / 2, 92, 18, "TEM CERTEZA QUE DESEJA", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
        Gfx_Text(dLCD_W / 2, 118, 18, "DESLIGAR O SISTEMA?", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
        Ui_DrawFrame(&dUI_RECT_YES, 12, (selectedItem == 0));
        Ui_DrawFrame(&dUI_RECT_NO, 12, (selectedItem == 1));
        Gfx_Text(dUI_RECT_YES.posX + (dUI_RECT_YES.width / 2), dUI_RECT_YES.posY + (dUI_RECT_YES.height / 2), 22, "SIM", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
        Gfx_Text(dUI_RECT_NO.posX + (dUI_RECT_NO.width / 2), dUI_RECT_NO.posY + (dUI_RECT_NO.height / 2), 22, "NAO", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
        break;
    }
}

/**************************************
* @brief Avanca para o proximo item navegavel da tela (botao NAV).
* @param screen: tela atual.
* @param selectedItemPtr: ponteiro para o indice do item selecionado.
* @retval Nenhum.
***************************************/
void Ui_Next(screen_t screen, int *selectedItemPtr)
{
    int count = Ui_GetItemCount(screen);
    *selectedItemPtr = (*selectedItemPtr + 1) % count;
}

/**************************************
* @brief Confirma a selecao do item atual (botao ENTER).
* @param screen: tela atual.
* @param selectedItemPtr: ponteiro para o indice do item selecionado.
* @param actionPtr: ponteiro para armazenar a acao executada.
* @retval Proxima tela a ser carregada.
***************************************/
screen_t Ui_Enter(screen_t screen, int *selectedItemPtr, uiAction_t *actionPtr)
{
    screen_t nextScreen = screen;
    *actionPtr = eUI_ACTION_NONE;

    switch(screen)
    {
    case eUI_SCREEN_HOME:
        if(*selectedItemPtr == 0)
        {
            nextScreen = eUI_SCREEN_BMP;
        }
        else if(*selectedItemPtr == 1)
        {
            nextScreen = eUI_SCREEN_MPU;
        }
        else
        {
            nextScreen = eUI_SCREEN_OFF;
        }
        break;

    case eUI_SCREEN_BMP:
    case eUI_SCREEN_MPU:
        nextScreen = eUI_SCREEN_HOME;
        break;

    case eUI_SCREEN_OFF:
        if(*selectedItemPtr == 0)
        {
            *actionPtr = eUI_ACTION_SHUTDOWN;
            return screen;
        }
        nextScreen = eUI_SCREEN_HOME;
        break;
    }

    if(nextScreen != screen)
    {
        *selectedItemPtr = Ui_GetStartSelection(screen, nextScreen);
    }

    return nextScreen;
}

/**************************************
* FUNCOES LOCAIS
***************************************/

/**************************************
* @brief Retorna a quantidade de itens navegaveis da tela.
* @param screen: identificador da tela.
* @retval Quantidade de itens.
***************************************/
static int Ui_GetItemCount(screen_t screen)
{
    switch(screen)
    {
    case eUI_SCREEN_HOME:
        return 3;
    case eUI_SCREEN_OFF:
        return 2;
    default:
        return 1;
    }
}

/**************************************
* @brief Determina o item inicial selecionado ao trocar de tela.
* @param fromScreen: tela de origem.
* @param toScreen: tela de destino.
* @retval Indice do item inicial.
***************************************/
static int Ui_GetStartSelection(screen_t fromScreen, screen_t toScreen)
{
    if(toScreen == eUI_SCREEN_HOME)
    {
        return (fromScreen == eUI_SCREEN_MPU) ? 1 : (fromScreen == eUI_SCREEN_OFF) ? 2 : 0;
    }

    if(toScreen == eUI_SCREEN_OFF)
    {
        return 1; /* Selecao padrao e NAO por seguranca */
    }

    return 0;
}

/**************************************
* @brief Desenha a moldura de um cartao com destaque se estiver selecionado.
* @param rectPtr: ponteiro para o retangulo do cartao.
* @param radius: raio dos cantos.
* @param isSelected: flag de selecao ativa.
* @retval Nenhum.
***************************************/
static void Ui_DrawFrame(const rect_t *rectPtr, int radius, bool isSelected)
{
    if(isSelected == true)
    {
        Gfx_FillRRect(rectPtr->posX, rectPtr->posY, rectPtr->width, rectPtr->height, radius, dUI_COLOR_GREEN);
        int innerRadius = (radius > 3) ? (radius - 3) : 1;
        Gfx_FillRRect(rectPtr->posX + 3, rectPtr->posY + 3, rectPtr->width - 6, rectPtr->height - 6, innerRadius, dUI_COLOR_CARD);
    }
    else
    {
        Gfx_FillRRect(rectPtr->posX, rectPtr->posY, rectPtr->width, rectPtr->height, radius, dUI_COLOR_CARD);
    }
}

/**************************************
* @brief Desenha o cabecalho padrao das telas.
* @param titleStr: titulo da tela.
* @param showBackButton: exibe o botao voltar se verdadeiro.
* @retval Nenhum.
***************************************/
static void Ui_DrawHeader(const char *titleStr, bool showBackButton)
{
    Gfx_Text(12, 22, 18, titleStr, dUI_COLOR_TEXT, eGFX_ALIGN_LEFT);
    if(showBackButton == true)
    {
        Ui_DrawFrame(&dUI_RECT_BACK, 10, true);
        Gfx_Text(dUI_RECT_BACK.posX + (dUI_RECT_BACK.width / 2), dUI_RECT_BACK.posY + (dUI_RECT_BACK.height / 2), 16, "< VOLTAR", dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
    }
}

/**************************************
* @brief Desenha um indicador LED de status.
* @param centerX: coordenada X central.
* @param centerY: coordenada Y central.
* @param isOnline: estado do LED (verde/vermelho).
* @retval Nenhum.
***************************************/
static void Ui_DrawLed(int centerX, int centerY, bool isOnline)
{
    Gfx_FillCircle(centerX, centerY, 6, (isOnline == true) ? dUI_COLOR_GREEN : dUI_COLOR_RED);
}

/**************************************
* @brief Desenha o icone do botao de desligar.
* @param centerX: coordenada X central.
* @param centerY: coordenada Y central.
* @retval Nenhum.
***************************************/
static void Ui_DrawPowerIcon(int centerX, int centerY)
{
    Gfx_Ring(centerX, centerY, 13, 13, 3, dUI_COLOR_GREEN);
    Gfx_FillRect(centerX - 6, centerY - 18, 12, 10, dUI_COLOR_CARD);
    Gfx_FillRRect(centerX - 2, centerY - 17, 4, 17, 2, dUI_COLOR_GREEN);
}

/**************************************
* @brief Desenha o icone estilizado de um circuito integrado (BMP280).
* @param boxPtr: retangulo delimitador do icone.
* @retval Nenhum.
***************************************/
static void Ui_DrawChipIcon(const rect_t *boxPtr)
{
    int centerX = boxPtr->posX + (boxPtr->width / 2);
    int centerY = boxPtr->posY + (boxPtr->height / 2);

    for(int i = -1; i <= 1; i++)
    {
        Gfx_FillRect(centerX - 30, centerY + (i * 12) - 1, 14, 3, dUI_COLOR_GREEN);
        Gfx_FillRect(centerX + 16, centerY + (i * 12) - 1, 14, 3, dUI_COLOR_GREEN);
        Gfx_FillRect(centerX + (i * 12) - 1, centerY - 30, 3, 14, dUI_COLOR_GREEN);
        Gfx_FillRect(centerX + (i * 12) - 1, centerY + 16, 3, 14, dUI_COLOR_GREEN);
    }

    Gfx_FillRRect(centerX - 18, centerY - 18, 36, 36, 5, dUI_COLOR_GREEN);
    Gfx_FillRRect(centerX - 16, centerY - 16, 32, 32, 4, Gfx_Rgb(30, 31, 38));
    Gfx_FillCircle(centerX - 9, centerY - 9, 2, dUI_COLOR_GREEN);
}

/**************************************
* @brief Desenha o icone estilizado de um atomo/inercial (MPU6050).
* @param boxPtr: retangulo delimitador do icone.
* @retval Nenhum.
***************************************/
static void Ui_DrawAtomIcon(const rect_t *boxPtr)
{
    int centerX = boxPtr->posX + (boxPtr->width / 2);
    int centerY = boxPtr->posY + (boxPtr->height / 2);

    Gfx_Ring(centerX, centerY, 30, 12, 3, dUI_COLOR_GREEN);
    Gfx_Ring(centerX, centerY, 12, 30, 3, dUI_COLOR_GREEN);
    Gfx_FillCircle(centerX, centerY, 4, dUI_COLOR_RED);
}

/**************************************
* @brief Desenha uma linha com legenda e caixa de valor medido.
* @param posY: posicao Y da linha.
* @param labelStr: texto da legenda.
* @param valueStr: texto do valor medido.
* @retval Nenhum.
***************************************/
static void Ui_DrawValueBox(int posY, const char *labelStr, const char *valueStr)
{
    Gfx_Text(104, posY + 17, 14, labelStr, dUI_COLOR_TEXT, eGFX_ALIGN_LEFT);
    Gfx_FillRRect(208, posY, 104, 34, 8, dUI_COLOR_CARD);
    Gfx_Text(260, posY + 17, 18, valueStr, dUI_COLOR_TEXT, eGFX_ALIGN_CENTER);
}

/**************************************
* @brief Desenha o rodape com a indicacao do estado geral do sensor.
* @param dataPtr: ponteiro para os dados do sensor.
* @retval Nenhum.
***************************************/
static void Ui_DrawStatusRow(const sensorData_t *dataPtr)
{
    Gfx_Text(12, 212, 18, "ESTADO:", dUI_COLOR_TEXT, eGFX_ALIGN_LEFT);

    if(dataPtr->isOnline == false)
    {
        Gfx_Text(110, 212, 18, "OFFLINE", dUI_COLOR_RED, eGFX_ALIGN_LEFT);
    }
    else if(strcmp(dataPtr->statusStr, "anormal") == 0)
    {
        Gfx_Text(110, 212, 18, "ANORMAL", dUI_COLOR_RED, eGFX_ALIGN_LEFT);
    }
    else
    {
        Gfx_Text(110, 212, 18, "NORMAL", dUI_COLOR_GREEN, eGFX_ALIGN_LEFT);
    }
}
/** @} DOXYGEN GROUP TAG END OF FILE */