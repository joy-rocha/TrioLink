/**************************************
* @file Ui.c
* @addtogroup Ui
* @brief Interface de utilizador e gestao de telas no display.
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
/// Cor de fundo padrao
#define dUI_C_BG    Gfx_Rgb(18, 18, 20)
/// Cor dos cartoes e caixas
#define dUI_C_CARD  Gfx_Rgb(38, 39, 50)
/// Cor do texto principal
#define dUI_C_TEXT  Gfx_Rgb(235, 235, 240)
/// Cor indicadora verde
#define dUI_C_GREEN Gfx_Rgb(25, 232, 138)
/// Cor indicadora vermelha
#define dUI_C_RED   Gfx_Rgb(255, 51, 85)

/**************************************
* CONSTANTES
***************************************/
/// Estrutura para delimitacao de retangulos de layout
typedef struct
{
    int posX;
    int posY;
    int width;
    int height;
} rect_t;

static const rect_t rCardBmp = { 12, 44, 144, 110 };
static const rect_t rCardMpu = { 164, 44, 144, 110 };
static const rect_t rPower = { 12, 166, 296, 62 };
static const rect_t rBack = { 204, 4, 108, 36 };
static const rect_t rYes = { 28, 150, 124, 56 };
static const rect_t rNo = { 168, 150, 124, 56 };

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static int Ui_GetItemsCount(screen_t screen);
static int Ui_GetStartSelection(screen_t fromScreen, screen_t toScreen);
static void Ui_DrawFrame(const rect_t *rectPtr, int radius, bool isSelected);
static void Ui_DrawHeader(const char *title, bool showBackButton);
static void Ui_DrawLed(int centerX, int centerY, bool isOnline);
static void Ui_DrawIconPower(int centerX, int centerY);
static void Ui_DrawIconChip(const rect_t *boxPtr);
static void Ui_DrawIconAtom(const rect_t *boxPtr);
static void Ui_DrawValueBox(int posY, const char *label, const char *value);
static void Ui_DrawStatusRow(const sensorData_t *sensorData);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
/** @brief Renderiza a tela atual no framebuffer.
* @param screen: identificador da tela ativa.
* @param selectedIndex: indice do item selecionado na tela.
* @param bmpData: ponteiro para os dados do sensor BMP.
* @param mpuData: ponteiro para os dados do sensor MPU.
* @retval Nenhum.
***************************************/
void Ui_Render(screen_t screen, int selectedIndex, const sensorData_t *bmpData, const sensorData_t *mpuData)
{
    char textBuffer[32];
    Gfx_Clear(dUI_C_BG);

    switch(screen)
    {
        case eSCREEN_HOME:
            Gfx_Text(dLCD_W / 2, 22, 18, "SISTEMA DE MONITORAMENTO", dUI_C_TEXT, eGFX_ALIGN_CENTER);
            Ui_DrawFrame(&rCardBmp, 12, selectedIndex == 0);
            Ui_DrawFrame(&rCardMpu, 12, selectedIndex == 1);
            Gfx_Text(rCardBmp.posX + (rCardBmp.width / 2), rCardBmp.posY + 58, 20, "BMP280", dUI_C_TEXT, eGFX_ALIGN_CENTER);
            Gfx_Text(rCardMpu.posX + (rCardMpu.width / 2), rCardMpu.posY + 58, 20, "MPU6050", dUI_C_TEXT, eGFX_ALIGN_CENTER);
            Ui_DrawLed(rCardBmp.posX + 20, rCardBmp.posY + 20, bmpData->online);
            Ui_DrawLed(rCardMpu.posX + 20, rCardMpu.posY + 20, mpuData->online);
            Ui_DrawFrame(&rPower, 12, selectedIndex == 2);
            Ui_DrawIconPower(rPower.posX + 40, rPower.posY + 31);
            Gfx_Text(rPower.posX + 70, rPower.posY + 31, 18, "DESLIGAR SISTEMA", dUI_C_TEXT, eGFX_ALIGN_LEFT);
            Gfx_Text(rPower.posX + rPower.width - 22, rPower.posY + 31, 20, ">", dUI_C_TEXT, eGFX_ALIGN_CENTER);
            break;

        case eSCREEN_BMP:
        {
            Ui_DrawHeader("BMP280", true);
            rect_t iconBox = { 10, 52, 84, 128 };
            Gfx_FillRRect(iconBox.posX, iconBox.posY, iconBox.width, iconBox.height, 12, dUI_C_CARD);
            Ui_DrawIconChip(&iconBox);
            snprintf(textBuffer, sizeof(textBuffer), "%.0f hPa", bmpData->value1);
            Ui_DrawValueBox(52, "PRESSAO:", textBuffer);
            snprintf(textBuffer, sizeof(textBuffer), "%.0f deg C", bmpData->value2);
            Ui_DrawValueBox(98, "TEMPERATURA:", textBuffer);
            snprintf(textBuffer, sizeof(textBuffer), "%.0f m", bmpData->value3);
            Ui_DrawValueBox(144, "ALTITUDE:", textBuffer);
            Ui_DrawStatusRow(bmpData);
            break;
        }

        case eSCREEN_MPU:
        {
            Ui_DrawHeader("MPU6050", true);
            rect_t iconBox = { 10, 52, 84, 128 };
            Gfx_FillRRect(iconBox.posX, iconBox.posY, iconBox.width, iconBox.height, 12, dUI_C_CARD);
            Ui_DrawIconAtom(&iconBox);
            snprintf(textBuffer, sizeof(textBuffer), "%.0f deg", mpuData->value1);
            Ui_DrawValueBox(72, "DIRECAO:", textBuffer);
            snprintf(textBuffer, sizeof(textBuffer), "%.1f m/s", mpuData->value2);
            Ui_DrawValueBox(126, "VELOCIDADE:", textBuffer);
            Ui_DrawStatusRow(mpuData);
            break;
        }

        case eSCREEN_OFF:
            Ui_DrawHeader("DESLIGAR SISTEMA", false);
            Gfx_Text(dLCD_W / 2, 92, 18, "TEM CERTEZA QUE DESEJA", dUI_C_TEXT, eGFX_ALIGN_CENTER);
            Gfx_Text(dLCD_W / 2, 118, 18, "DESLIGAR O SISTEMA?", dUI_C_TEXT, eGFX_ALIGN_CENTER);
            Ui_DrawFrame(&rYes, 12, selectedIndex == 0);
            Ui_DrawFrame(&rNo, 12, selectedIndex == 1);
            Gfx_Text(rYes.posX + (rYes.width / 2), rYes.posY + (rYes.height / 2), 22, "SIM", dUI_C_TEXT, eGFX_ALIGN_CENTER);
            Gfx_Text(rNo.posX + (rNo.width / 2), rNo.posY + (rNo.height / 2), 22, "NAO", dUI_C_TEXT, eGFX_ALIGN_CENTER);
            break;
    }
}

/**************************************
/** @brief Avanca a selecao para o proximo item da tela atual.
* @param screen: identificador da tela ativa.
* @param selectedIndex: ponteiro para o indice selecionado.
* @retval Nenhum.
***************************************/
void Ui_Next(screen_t screen, int *selectedIndex)
{
    *selectedIndex = (*selectedIndex + 1) % Ui_GetItemsCount(screen);
}

/**************************************
/** @brief Processa a acao de confirmacao no item selecionado.
* @param screen: identificador da tela atual.
* @param selectedIndex: ponteiro para o indice selecionado.
* @param action: ponteiro para armazenar a acao resultante.
* @retval Nova tela a ser exibida.
***************************************/
screen_t Ui_Enter(screen_t screen, int *selectedIndex, uiAction_t *action)
{
    screen_t nextScreen = screen;
    *action = eUI_ACTION_NONE;

    switch(screen)
    {
        case eSCREEN_HOME:
            if(*selectedIndex == 0)
            {
                nextScreen = eSCREEN_BMP;
            }
            else if(*selectedIndex == 1)
            {
                nextScreen = eSCREEN_MPU;
            }
            else
            {
                nextScreen = eSCREEN_OFF;
            }
            break;

        case eSCREEN_BMP:
        case eSCREEN_MPU:
            nextScreen = eSCREEN_HOME;
            break;

        case eSCREEN_OFF:
            if(*selectedIndex == 0)
            {
                *action = eUI_ACTION_SHUTDOWN;
                return screen;
            }
            nextScreen = eSCREEN_HOME;
            break;
    }

    if(nextScreen != screen)
    {
        *selectedIndex = Ui_GetStartSelection(screen, nextScreen);
    }

    return nextScreen;
}

/**************************************
* FUNCOES LOCAIS
***************************************/

/**************************************
/** @brief Retorna a quantidade de itens selecionaveis de uma tela.
* @param screen: identificador da tela.
* @retval Quantidade de itens.
***************************************/
static int Ui_GetItemsCount(screen_t screen)
{
    switch(screen)
    {
        case eSCREEN_HOME:
            return 3;
        case eSCREEN_OFF:
            return 2;
        default:
            return 1;
    }
}

/**************************************
/** @brief Retorna o item inicialmente selecionado ao transitar de tela.
* @param fromScreen: tela de origem.
* @param toScreen: tela de destino.
* @retval Indice de selecao inicial.
***************************************/
static int Ui_GetStartSelection(screen_t fromScreen, screen_t toScreen)
{
    if(toScreen == eSCREEN_HOME)
    {
        if(fromScreen == eSCREEN_MPU)
        {
            return 1;
        }
        if(fromScreen == eSCREEN_OFF)
        {
            return 2;
        }
        return 0;
    }

    if(toScreen == eSCREEN_OFF)
    {
        return 1;
    }

    return 0;
}

/**************************************
/** @brief Desenha a moldura de selecao de um componente de tela.
* @param rectPtr: ponteiro para o retangulo de layout.
* @param radius: raio das bordas.
* @param isSelected: true se o item estiver selecionado.
* @retval Nenhum.
***************************************/
static void Ui_DrawFrame(const rect_t *rectPtr, int radius, bool isSelected)
{
    if(isSelected == true)
    {
        Gfx_FillRRect(rectPtr->posX, rectPtr->posY, rectPtr->width, rectPtr->height, radius, dUI_C_GREEN);
        int innerRadius = (radius > 3) ? (radius - 3) : 1;
        Gfx_FillRRect(rectPtr->posX + 3, rectPtr->posY + 3, rectPtr->width - 6, rectPtr->height - 6, innerRadius, dUI_C_CARD);
    }
    else
    {
        Gfx_FillRRect(rectPtr->posX, rectPtr->posY, rectPtr->width, rectPtr->height, radius, dUI_C_CARD);
    }
}

/**************************************
/** @brief Desenha o cabecalho padrao da tela.
* @param title: titulo da tela.
* @param showBackButton: true para exibir o botao voltar.
* @retval Nenhum.
***************************************/
static void Ui_DrawHeader(const char *title, bool showBackButton)
{
    Gfx_Text(12, 22, 18, title, dUI_C_TEXT, eGFX_ALIGN_LEFT);
    if(showBackButton == true)
    {
        Ui_DrawFrame(&rBack, 10, true);
        Gfx_Text(rBack.posX + (rBack.width / 2), rBack.posY + (rBack.height / 2), 16, "< VOLTAR", dUI_C_TEXT, eGFX_ALIGN_CENTER);
    }
}

/**************************************
/** @brief Desenha um indicador LED circular de estado.
* @param centerX: coordenada X.
* @param centerY: coordenada Y.
* @param isOnline: true para verde (online), false para vermelho (offline).
* @retval Nenhum.
***************************************/
static void Ui_DrawLed(int centerX, int centerY, bool isOnline)
{
    Gfx_FillCircle(centerX, centerY, 6, (isOnline == true) ? dUI_C_GREEN : dUI_C_RED);
}

/**************************************
/** @brief Desenha o icone de energia/desligar.
* @param centerX: coordenada X.
* @param centerY: coordenada Y.
* @retval Nenhum.
***************************************/
static void Ui_DrawIconPower(int centerX, int centerY)
{
    Gfx_Ring(centerX, centerY, 13, 13, 3, dUI_C_GREEN);
    Gfx_FillRect(centerX - 6, centerY - 18, 12, 10, dUI_C_CARD);
    Gfx_FillRRect(centerX - 2, centerY - 17, 4, 17, 2, dUI_C_GREEN);
}

/**************************************
/** @brief Desenha o icone do chip CI.
* @param boxPtr: retangulo delimitador do icone.
* @retval Nenhum.
***************************************/
static void Ui_DrawIconChip(const rect_t *boxPtr)
{
    int centerX = boxPtr->posX + (boxPtr->width / 2);
    int centerY = boxPtr->posY + (boxPtr->height / 2);

    for(int i = -1; i <= 1; i++)
    {
        Gfx_FillRect(centerX - 30, centerY + (i * 12) - 1, 14, 3, dUI_C_GREEN);
        Gfx_FillRect(centerX + 16, centerY + (i * 12) - 1, 14, 3, dUI_C_GREEN);
        Gfx_FillRect(centerX + (i * 12) - 1, centerY - 30, 3, 14, dUI_C_GREEN);
        Gfx_FillRect(centerX + (i * 12) - 1, centerY + 16, 3, 14, dUI_C_GREEN);
    }

    Gfx_FillRRect(centerX - 18, centerY - 18, 36, 36, 5, dUI_C_GREEN);
    Gfx_FillRRect(centerX - 16, centerY - 16, 32, 32, 4, Gfx_Rgb(30, 31, 38));
    Gfx_FillCircle(centerX - 9, centerY - 9, 2, dUI_C_GREEN);
}

/**************************************
/** @brief Desenha o icone do atomo.
* @param boxPtr: retangulo delimitador do icone.
* @retval Nenhum.
***************************************/
static void Ui_DrawIconAtom(const rect_t *boxPtr)
{
    int centerX = boxPtr->posX + (boxPtr->width / 2);
    int centerY = boxPtr->posY + (boxPtr->height / 2);

    Gfx_Ring(centerX, centerY, 30, 12, 3, dUI_C_GREEN);
    Gfx_Ring(centerX, centerY, 12, 30, 3, dUI_C_GREEN);
    Gfx_FillCircle(centerX, centerY, 4, dUI_C_RED);
}

/**************************************
/** @brief Desenha uma caixa de valor com rotulo na interface.
* @param posY: coordenada Y da caixa.
* @param label: texto do rotulo.
* @param value: texto do valor.
* @retval Nenhum.
***************************************/
static void Ui_DrawValueBox(int posY, const char *label, const char *value)
{
    Gfx_Text(104, posY + 17, 14, label, dUI_C_TEXT, eGFX_ALIGN_LEFT);
    Gfx_FillRRect(208, posY, 104, 34, 8, dUI_C_CARD);
    Gfx_Text(260, posY + 17, 18, value, dUI_C_TEXT, eGFX_ALIGN_CENTER);
}

/**************************************
/** @brief Exibe a linha indicadora de estado do sensor.
* @param sensorData: ponteiro para os dados do sensor.
* @retval Nenhum.
***************************************/
static void Ui_DrawStatusRow(const sensorData_t *sensorData)
{
    Gfx_Text(12, 212, 18, "ESTADO:", dUI_C_TEXT, eGFX_ALIGN_LEFT);
    if(sensorData->online == false)
    {
        Gfx_Text(110, 212, 18, "OFFLINE", dUI_C_RED, eGFX_ALIGN_LEFT);
    }
    else if(strcmp(sensorData->status, "anormal") == 0)
    {
        Gfx_Text(110, 212, 18, "ANORMAL", dUI_C_RED, eGFX_ALIGN_LEFT);
    }
    else
    {
        Gfx_Text(110, 212, 18, "NORMAL", dUI_C_GREEN, eGFX_ALIGN_LEFT);
    }
}
/** @} DOXYGEN GROUP TAG END OF FILE */