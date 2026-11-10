/*
 * @file    BspScreen.c
 * @brief   Interface desenhada com Cairo em uma superficie RGB565 de 320x240.
 *          A mesma superficie e enviada ao ILI9341 (barramento paralelo de
 *          8 bits via BspGpio) e exibida em uma janela GTK4.
 */

#include "BspScreen.h"

#include "BspButton.h"
#include "BspGpio.h"
#include "Fusion.h"

#include <cairo.h>
#include <glib-unix.h>
#include <gtk/gtk.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/// Janela em tela cheia (1) ou 320x240 sem redimensionar (0)
#define dSCREEN_FULLSCREEN              0

/// Resolucao da interface (paisagem) [pixels]
#define dSCREEN_WIDTH                   320
#define dSCREEN_HEIGHT                  240

/// Registrador MADCTL do ILI9341: 0x28 = paisagem (MV) com ordem BGR.
/// Se a imagem sair espelhada, ajustar os bits MX (0x40) e MY (0x80)
#define dSCREEN_MADCTL                  0x68

/// Periodo de atualizacao dos dados na tela [ms]
#define dSCREEN_REFRESH_MS              200

/// Titulo da janela GTK
#define dSCREEN_WINDOW_TITLE            "Cascata de Sensores - TRIOLIKN"

/// Comandos do ILI9341
#define dILI_CMD_SOFT_RESET             0x01
#define dILI_CMD_SLEEP_OUT              0x11
#define dILI_CMD_DISPLAY_ON             0x29
#define dILI_CMD_COLUMN_ADDR            0x2A
#define dILI_CMD_PAGE_ADDR              0x2B
#define dILI_CMD_MEMORY_WRITE           0x2C
#define dILI_CMD_MADCTL                 0x36
#define dILI_CMD_PIXEL_FORMAT           0x3A

/// Formato de pixel RGB565 (16 bits)
#define dILI_PIXEL_FORMAT_16BIT         0x55

/// Maximo de parametros de um passo da sequencia de init
#define dILI_INIT_MAX_PARAMS            15

/// Tempos de espera do display [us]
#define dILI_RESET_PULSE_US             10000
#define dILI_RESET_RECOVERY_US          120000
#define dILI_SLEEP_OUT_DELAY_US         120000

/// Manipulacao do pixel de 16 bits
#define dBYTE_SHIFT                     8
#define dBYTE_MASK                      0xFF

/// Fontes [pixels]
#define dFONT_FAMILY                    "Sans"
#define dFONT_SIZE_TITLE                16.0
#define dFONT_SIZE_MENU_BUTTON          22.0
#define dFONT_SIZE_BUTTON               15.0
#define dFONT_SIZE_VALUE                15.0
#define dFONT_SIZE_INFO                 14.0

/// Formas
#define dLINE_WIDTH                     2.0
#define dLINE_WIDTH_SELECTED            4.0
#define dCORNER_RADIUS                  8.0
#define dDOT_RADIUS                     5.0
#define dFULL_CIRCLE                    6.283185307179586

/// Textos
#define dTEXT_BUFFER_SIZE               32
#define dTEXT_NO_DATA                   "---"
#define dFORMAT_PRESSURE                "%.0f Pa"
#define dFORMAT_TEMPERATURE             "%.2f °C"
#define dFORMAT_ACCEL                   "%.2f m/s²"

/// Layout da tela inicial
#define dMENU_TITLE_Y                   30.0

/// Layout das telas dos sensores
#define dSENSOR_TITLE_X                 16.0
#define dSENSOR_TITLE_Y                 24.0
#define dSENSOR_LINK_Y                  58.0
#define dSENSOR_DOT_X                   22.0
#define dSENSOR_LINK_TEXT_X             34.0
#define dSENSOR_ROW_FIRST_Y             78.0
#define dSENSOR_ROW_STEP_Y              42.0
#define dSENSOR_ROW_HEIGHT              32.0
#define dSENSOR_LABEL_X                 16.0
#define dSENSOR_VALUE_X                 140.0
#define dSENSOR_VALUE_WIDTH             164.0
#define dSENSOR_STATE_Y                 218.0
#define dSENSOR_STATE_VALUE_X           84.0

/// Botoes que existem na tela inicial
typedef enum
{
    eMENU_BUTTON_MPU = 0,
    eMENU_BUTTON_BMP,
    eMENU_BUTTON_COUNT
} menuButton_t;

/// Telas do sistema
typedef enum
{
    eSCREEN_MENU = 0,
    eSCREEN_MPU,
    eSCREEN_BMP
} screenId_t;

/// Cor com componentes de 0 a 1
typedef struct
{
    double r;
    double g;
    double b;
} color_t;

/// Retangulo de um elemento de tela [pixels]
typedef struct
{
    double x;
    double y;
    double width;
    double height;
} rect_t;

/// Passo da sequencia de inicializacao do ILI9341
typedef struct
{
    uint8_t command;
    uint8_t numParams;
    uint8_t params[dILI_INIT_MAX_PARAMS];
} iliInitStep_t;

/// Paleta da interface (tema escuro)
static const color_t colorBackground = { 0.07, 0.07, 0.07 };
static const color_t colorForeground = { 0.93, 0.93, 0.93 };
static const color_t colorBorder     = { 0.55, 0.55, 0.55 };
static const color_t colorAccent     = { 0.15, 0.45, 0.90 };
static const color_t colorOk         = { 0.20, 0.80, 0.40 };
static const color_t colorError      = { 0.90, 0.20, 0.20 };
static const color_t colorWarning    = { 0.96, 0.75, 0.10 };

/// Botoes da tela inicial, indexados por menuButton_t
static const rect_t menuButtonRects[eMENU_BUTTON_COUNT] =
{
    [eMENU_BUTTON_MPU] = { 24.0,  80.0, 128.0, 90.0 },
    [eMENU_BUTTON_BMP] = { 168.0, 80.0, 128.0, 90.0 }
};

/// Rotulos dos botoes da tela inicial, indexados por menuButton_t
static const char *const menuButtonLabels[eMENU_BUTTON_COUNT] =
{
    [eMENU_BUTTON_MPU] = "MPU",
    [eMENU_BUTTON_BMP] = "BMP"
};

/// Botao VOLTAR das telas dos sensores (sempre selecionado)
static const rect_t backButtonRect = { 212.0, 8.0, 100.0, 32.0 };

/// Sequencia de inicializacao do ILI9341 (modulo 2.4" LCDwiki)
static const iliInitStep_t iliInitSteps[] =
{
    { 0xCF, 3,  { 0x00, 0xC1, 0x30 } },
    { 0xED, 4,  { 0x64, 0x03, 0x12, 0x81 } },
    { 0xE8, 3,  { 0x85, 0x00, 0x78 } },
    { 0xCB, 5,  { 0x39, 0x2C, 0x00, 0x34, 0x02 } },
    { 0xF7, 1,  { 0x20 } },
    { 0xEA, 2,  { 0x00, 0x00 } },
    { 0xC0, 1,  { 0x1B } },
    { 0xC1, 1,  { 0x01 } },
    { 0xC5, 2,  { 0x30, 0x30 } },
    { 0xC7, 1,  { 0xB7 } },
    { dILI_CMD_MADCTL, 1, { dSCREEN_MADCTL } },
    { dILI_CMD_PIXEL_FORMAT, 1, { dILI_PIXEL_FORMAT_16BIT } },
    { 0xB1, 2,  { 0x00, 0x1A } },
    { 0xB6, 2,  { 0x0A, 0xA2 } },
    { 0xF2, 1,  { 0x00 } },
    { 0x26, 1,  { 0x01 } },
    { 0xE0, 15, { 0x0F, 0x2A, 0x28, 0x08, 0x0E, 0x08, 0x54, 0xA9,
                  0x43, 0x0A, 0x0F, 0x00, 0x00, 0x00, 0x00 } },
    { 0xE1, 15, { 0x00, 0x15, 0x17, 0x07, 0x11, 0x06, 0x2B, 0x56,
                  0x3C, 0x05, 0x10, 0x0F, 0x3F, 0x3F, 0x0F } }
};

/// Dados locais do modulo
static struct
{
    GMainLoop *loop;
    GtkWidget *window;          // NULL se nao houver sessao grafica
    GtkWidget *drawingArea;     // NULL se nao houver sessao grafica
    cairo_surface_t *surface;
    uint16_t previousFrame[dSCREEN_HEIGHT][dSCREEN_WIDTH];
    bool frameValid;            // true = previousFrame reflete o display
    screenId_t screen;
    menuButton_t selected;
} bspScreen;

/*
 * @brief   Envia um byte ao barramento de dados e gera o pulso de escrita.
 * @param   value - Byte a ser enviado
 * @retval  void
 */
static void BspScreen_PanelWriteByte(uint8_t value)
{
    (void)BspGpio_WriteDataBus(value);
    (void)BspGpio_Write(eBSP_GPIO_LCD_WR, false);
    (void)BspGpio_Write(eBSP_GPIO_LCD_WR, true);
}

/*
 * @brief   Envia um comando ao ILI9341 (RS baixo).
 * @param   command - Codigo do comando
 * @retval  void
 */
static void BspScreen_PanelWriteCommand(uint8_t command)
{
    (void)BspGpio_Write(eBSP_GPIO_LCD_RS, false);
    (void)BspGpio_Write(eBSP_GPIO_LCD_CS, false);
    BspScreen_PanelWriteByte(command);
    (void)BspGpio_Write(eBSP_GPIO_LCD_CS, true);
}

/*
 * @brief   Envia um dado ao ILI9341 (RS alto).
 * @param   data - Byte de dado
 * @retval  void
 */
static void BspScreen_PanelWriteData(uint8_t data)
{
    (void)BspGpio_Write(eBSP_GPIO_LCD_RS, true);
    (void)BspGpio_Write(eBSP_GPIO_LCD_CS, false);
    BspScreen_PanelWriteByte(data);
    (void)BspGpio_Write(eBSP_GPIO_LCD_CS, true);
}

/*
 * @brief   Reseta e configura o ILI9341 (paisagem, RGB565).
 * @retval  void
 */
static void BspScreen_PanelInit(void)
{
    size_t step;
    uint8_t param;

    // Todas as saidas iniciam em nivel baixo; WR, RD e CS ficam em repouso (alto)
    (void)BspGpio_Write(eBSP_GPIO_LCD_WR, true);
    (void)BspGpio_Write(eBSP_GPIO_LCD_RD, true);
    (void)BspGpio_Write(eBSP_GPIO_LCD_CS, true);

    (void)BspGpio_Write(eBSP_GPIO_LCD_RST, true);
    usleep(dILI_RESET_PULSE_US);
    (void)BspGpio_Write(eBSP_GPIO_LCD_RST, false);
    usleep(dILI_RESET_PULSE_US);
    (void)BspGpio_Write(eBSP_GPIO_LCD_RST, true);
    usleep(dILI_RESET_RECOVERY_US);

    BspScreen_PanelWriteCommand(dILI_CMD_SOFT_RESET);
    usleep(dILI_RESET_RECOVERY_US);

    for(step = 0; step < (sizeof(iliInitSteps) / sizeof(iliInitSteps[0])); step++)
    {
        BspScreen_PanelWriteCommand(iliInitSteps[step].command);

        for(param = 0; param < iliInitSteps[step].numParams; param++)
        {
            BspScreen_PanelWriteData(iliInitSteps[step].params[param]);
        }
    }

    BspScreen_PanelWriteCommand(dILI_CMD_SLEEP_OUT);
    usleep(dILI_SLEEP_OUT_DELAY_US);
    BspScreen_PanelWriteCommand(dILI_CMD_DISPLAY_ON);

    bspScreen.frameValid = false;
}

/*
 * @brief   Define a janela de escrita da memoria do display e abre a escrita.
 * @param   x0 - Coluna inicial [pixels]
 * @param   y0 - Linha inicial [pixels]
 * @param   x1 - Coluna final, inclusive [pixels]
 * @param   y1 - Linha final, inclusive [pixels]
 * @retval  void
 */
static void BspScreen_PanelSetWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    BspScreen_PanelWriteCommand(dILI_CMD_COLUMN_ADDR);
    BspScreen_PanelWriteData((uint8_t)(x0 >> dBYTE_SHIFT));
    BspScreen_PanelWriteData((uint8_t)(x0 & dBYTE_MASK));
    BspScreen_PanelWriteData((uint8_t)(x1 >> dBYTE_SHIFT));
    BspScreen_PanelWriteData((uint8_t)(x1 & dBYTE_MASK));

    BspScreen_PanelWriteCommand(dILI_CMD_PAGE_ADDR);
    BspScreen_PanelWriteData((uint8_t)(y0 >> dBYTE_SHIFT));
    BspScreen_PanelWriteData((uint8_t)(y0 & dBYTE_MASK));
    BspScreen_PanelWriteData((uint8_t)(y1 >> dBYTE_SHIFT));
    BspScreen_PanelWriteData((uint8_t)(y1 & dBYTE_MASK));

    BspScreen_PanelWriteCommand(dILI_CMD_MEMORY_WRITE);
}

/*
 * @brief   Envia ao display apenas o retangulo que mudou desde o ultimo
 *          quadro. O envio e por bit-banging, entao transmitir so a regiao
 *          alterada mantem os botoes responsivos.
 * @retval  void
 */
static void BspScreen_PanelFlush(void)
{
    const uint8_t *data;
    const uint16_t *row;
    int stride;
    uint16_t x;
    uint16_t y;
    uint16_t minX = dSCREEN_WIDTH;
    uint16_t minY = dSCREEN_HEIGHT;
    uint16_t maxX = 0;
    uint16_t maxY = 0;
    bool hasChange = false;

    cairo_surface_flush(bspScreen.surface);
    data = cairo_image_surface_get_data(bspScreen.surface);
    stride = cairo_image_surface_get_stride(bspScreen.surface);

    for(y = 0; y < dSCREEN_HEIGHT; y++)
    {
        row = (const uint16_t *)(data + ((size_t)y * (size_t)stride));

        for(x = 0; x < dSCREEN_WIDTH; x++)
        {
            if((bspScreen.frameValid == false) || (row[x] != bspScreen.previousFrame[y][x]))
            {
                hasChange = true;
                minX = (x < minX) ? x : minX;
                maxX = (x > maxX) ? x : maxX;
                minY = (y < minY) ? y : minY;
                maxY = (y > maxY) ? y : maxY;
            }
        }
    }

    if(hasChange == false)
    {
        return;
    }

    BspScreen_PanelSetWindow(minX, minY, maxX, maxY);

    (void)BspGpio_Write(eBSP_GPIO_LCD_RS, true);
    (void)BspGpio_Write(eBSP_GPIO_LCD_CS, false);

    for(y = minY; y <= maxY; y++)
    {
        row = (const uint16_t *)(data + ((size_t)y * (size_t)stride));

        for(x = minX; x <= maxX; x++)
        {
            BspScreen_PanelWriteByte((uint8_t)(row[x] >> dBYTE_SHIFT));
            BspScreen_PanelWriteByte((uint8_t)(row[x] & dBYTE_MASK));
            bspScreen.previousFrame[y][x] = row[x];
        }
    }

    (void)BspGpio_Write(eBSP_GPIO_LCD_CS, true);
    bspScreen.frameValid = true;
}

/*
 * @brief   Define a cor de desenho atual.
 * @param   cr    - Contexto Cairo
 * @param   color - Cor desejada
 * @retval  void
 */
static void BspScreen_SetColor(cairo_t *cr, const color_t *color)
{
    cairo_set_source_rgb(cr, color->r, color->g, color->b);
}

/*
 * @brief   Desenha um texto com a linha de base padronizada, para que textos
 *          diferentes na mesma linha nao "pulem" na vertical.
 * @param   cr         - Contexto Cairo
 * @param   text       - Texto UTF-8
 * @param   x          - Centro (se centralizado) ou inicio do texto [pixels]
 * @param   centerY    - Centro vertical [pixels]
 * @param   size       - Tamanho da fonte [pixels]
 * @param   isBold     - true para negrito
 * @param   isCentered - true para centralizar horizontalmente em x
 * @param   color      - Cor do texto
 * @retval  void
 */
static void BspScreen_DrawText(cairo_t *cr,
                               const char *text,
                               double x,
                               double centerY,
                               double size,
                               bool isBold,
                               bool isCentered,
                               const color_t *color)
{
    cairo_text_extents_t extents;
    cairo_font_extents_t fontExtents;
    double textX = x;
    double baseline;

    cairo_select_font_face(cr, dFONT_FAMILY, CAIRO_FONT_SLANT_NORMAL,
                           (isBold == true) ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
    cairo_font_extents(cr, &fontExtents);
    cairo_text_extents(cr, text, &extents);

    if(isCentered == true)
    {
        textX = x - ((extents.width / 2.0) + extents.x_bearing);
    }

    baseline = centerY + ((fontExtents.ascent - fontExtents.descent) / 2.0);

    BspScreen_SetColor(cr, color);
    cairo_move_to(cr, textX, baseline);
    cairo_show_text(cr, text);
}

/*
 * @brief   Cria o caminho de um retangulo de cantos arredondados.
 * @param   cr   - Contexto Cairo
 * @param   rect - Posicao e tamanho do retangulo
 * @retval  void
 */
static void BspScreen_RoundedRect(cairo_t *cr, const rect_t *rect)
{
    const double quarter = G_PI / 2.0;
    const double radius = dCORNER_RADIUS;

    cairo_new_sub_path(cr);
    cairo_arc(cr, rect->x + rect->width - radius, rect->y + radius,
              radius, -quarter, 0.0);
    cairo_arc(cr, rect->x + rect->width - radius, rect->y + rect->height - radius,
              radius, 0.0, quarter);
    cairo_arc(cr, rect->x + radius, rect->y + rect->height - radius,
              radius, quarter, 2.0 * quarter);
    cairo_arc(cr, rect->x + radius, rect->y + radius,
              radius, 2.0 * quarter, 3.0 * quarter);
    cairo_close_path(cr);
}

/*
 * @brief   Desenha um botao arredondado com o rotulo centralizado.
 *          O botao selecionado ganha borda azul.
 * @param   cr         - Contexto Cairo
 * @param   rect       - Posicao e tamanho do botao
 * @param   label      - Texto do botao
 * @param   fontSize   - Tamanho da fonte do rotulo [pixels]
 * @param   isSelected - true para destacar o botao
 * @retval  void
 */
static void BspScreen_DrawButton(cairo_t *cr,
                                 const rect_t *rect,
                                 const char *label,
                                 double fontSize,
                                 bool isSelected)
{
    BspScreen_RoundedRect(cr, rect);
    BspScreen_SetColor(cr, (isSelected == true) ? &colorAccent : &colorBorder);
    cairo_set_line_width(cr, (isSelected == true) ? dLINE_WIDTH_SELECTED : dLINE_WIDTH);
    cairo_stroke(cr);

    BspScreen_DrawText(cr, label, rect->x + (rect->width / 2.0),
                       rect->y + (rect->height / 2.0), fontSize, true, true,
                       &colorForeground);
}

/*
 * @brief   Desenha o cabecalho das telas dos sensores: titulo, botao VOLTAR
 *          (sempre selecionado) e indicador de conexao.
 * @param   cr        - Contexto Cairo
 * @param   title     - Nome do sensor
 * @param   connected - true = ONLINE, false = OFFLINE
 * @retval  void
 */
static void BspScreen_DrawSensorHeader(cairo_t *cr, const char *title, bool connected)
{
    const color_t *color = (connected == true) ? &colorOk : &colorError;

    BspScreen_DrawText(cr, title, dSENSOR_TITLE_X, dSENSOR_TITLE_Y,
                       dFONT_SIZE_TITLE, true, false, &colorForeground);
    BspScreen_DrawButton(cr, &backButtonRect, "VOLTAR", dFONT_SIZE_BUTTON, true);

    BspScreen_SetColor(cr, color);
    cairo_arc(cr, dSENSOR_DOT_X, dSENSOR_LINK_Y, dDOT_RADIUS, 0.0, dFULL_CIRCLE);
    cairo_fill(cr);

    BspScreen_DrawText(cr, (connected == true) ? "ONLINE" : "OFFLINE",
                       dSENSOR_LINK_TEXT_X, dSENSOR_LINK_Y, dFONT_SIZE_INFO,
                       true, false, color);
}

/*
 * @brief   Desenha uma linha "rotulo: [valor]" das telas dos sensores.
 * @param   cr    - Contexto Cairo
 * @param   row   - Indice da linha (0 a 2)
 * @param   label - Rotulo a esquerda
 * @param   value - Texto exibido dentro da caixa
 * @retval  void
 */
static void BspScreen_DrawValueRow(cairo_t *cr, uint8_t row, const char *label, const char *value)
{
    const double rowY = dSENSOR_ROW_FIRST_Y + ((double)row * dSENSOR_ROW_STEP_Y);
    const double centerY = rowY + (dSENSOR_ROW_HEIGHT / 2.0);
    const rect_t box = { dSENSOR_VALUE_X, rowY, dSENSOR_VALUE_WIDTH, dSENSOR_ROW_HEIGHT };

    BspScreen_DrawText(cr, label, dSENSOR_LABEL_X, centerY, dFONT_SIZE_VALUE,
                       false, false, &colorForeground);

    BspScreen_RoundedRect(cr, &box);
    BspScreen_SetColor(cr, &colorBorder);
    cairo_set_line_width(cr, dLINE_WIDTH);
    cairo_stroke(cr);

    BspScreen_DrawText(cr, value, box.x + (box.width / 2.0), centerY,
                       dFONT_SIZE_VALUE, false, true, &colorForeground);
}

/*
 * @brief   Desenha o estado do sensor no rodape.
 *          Prioridade: SEM DADOS > DADO OBSOLETO > ANORMAL > NORMAL.
 * @param   cr      - Contexto Cairo
 * @param   hasData - true se ja houve alguma leitura
 * @param   stale   - true se o dado esta obsoleto
 * @param   health  - Classificacao feita pelo Fusion
 * @retval  void
 */
static void BspScreen_DrawState(cairo_t *cr, bool hasData, bool stale, fusionHealth_t health)
{
    const char *text = "NORMAL";
    const color_t *color = &colorOk;

    if(hasData == false)
    {
        text = "SEM DADOS";
        color = &colorWarning;
    }
    else if(stale == true)
    {
        text = "DADO OBSOLETO";
        color = &colorWarning;
    }
    else if(health != eFUSION_HEALTH_NORMAL)
    {
        text = "ANORMAL";
        color = &colorError;
    }

    BspScreen_DrawText(cr, "estado:", dSENSOR_LABEL_X, dSENSOR_STATE_Y,
                       dFONT_SIZE_VALUE, false, false, &colorForeground);
    BspScreen_DrawText(cr, text, dSENSOR_STATE_VALUE_X, dSENSOR_STATE_Y,
                       dFONT_SIZE_VALUE, true, false, color);
}

/*
 * @brief   Desenha a tela inicial: titulo e os botoes MPU e BMP.
 * @param   cr - Contexto Cairo
 * @retval  void
 */
static void BspScreen_DrawMenu(cairo_t *cr)
{
    uint8_t button;

    BspScreen_DrawText(cr, dSCREEN_WINDOW_TITLE, dSCREEN_WIDTH / 2.0, dMENU_TITLE_Y,
                       dFONT_SIZE_TITLE, true, true, &colorForeground);

    for(button = 0; button < eMENU_BUTTON_COUNT; button++)
    {
        BspScreen_DrawButton(cr, &menuButtonRects[button], menuButtonLabels[button],
                             dFONT_SIZE_MENU_BUTTON,
                             (bool)(bspScreen.selected == (menuButton_t)button));
    }
}

/*
 * @brief   Desenha a tela do BMP: pressao, temperatura e estado.
 * @param   cr - Contexto Cairo
 * @retval  void
 */
static void BspScreen_DrawBmp(cairo_t *cr)
{
    fusionBmp_t bmp;
    char pressure[dTEXT_BUFFER_SIZE] = dTEXT_NO_DATA;
    char temperature[dTEXT_BUFFER_SIZE] = dTEXT_NO_DATA;

    (void)memset(&bmp, 0, sizeof(bmp));
    (void)Fusion_GetBmp(&bmp);

    if(bmp.hasData == true)
    {
        (void)snprintf(pressure, sizeof(pressure), dFORMAT_PRESSURE, (double)bmp.pressure);
        (void)snprintf(temperature, sizeof(temperature), dFORMAT_TEMPERATURE,
                       (double)bmp.temperature);
    }

    BspScreen_DrawSensorHeader(cr, "BMP", bmp.connected);
    BspScreen_DrawValueRow(cr, 0, "pressão:", pressure);
    BspScreen_DrawValueRow(cr, 1, "temperatura:", temperature);
    BspScreen_DrawState(cr, bmp.hasData, bmp.stale, bmp.health);
}

/*
 * @brief   Desenha a tela do MPU: aceleracao em x, y e z e estado.
 * @param   cr - Contexto Cairo
 * @retval  void
 */
static void BspScreen_DrawMpu(cairo_t *cr)
{
    fusionMpu_t mpu;
    char axisX[dTEXT_BUFFER_SIZE] = dTEXT_NO_DATA;
    char axisY[dTEXT_BUFFER_SIZE] = dTEXT_NO_DATA;
    char axisZ[dTEXT_BUFFER_SIZE] = dTEXT_NO_DATA;

    (void)memset(&mpu, 0, sizeof(mpu));
    (void)Fusion_GetMpu(&mpu);

    if(mpu.hasData == true)
    {
        (void)snprintf(axisX, sizeof(axisX), dFORMAT_ACCEL, (double)mpu.accelX);
        (void)snprintf(axisY, sizeof(axisY), dFORMAT_ACCEL, (double)mpu.accelY);
        (void)snprintf(axisZ, sizeof(axisZ), dFORMAT_ACCEL, (double)mpu.accelZ);
    }

    BspScreen_DrawSensorHeader(cr, "MPU", mpu.connected);
    BspScreen_DrawValueRow(cr, 0, "x:", axisX);
    BspScreen_DrawValueRow(cr, 1, "y:", axisY);
    BspScreen_DrawValueRow(cr, 2, "z:", axisZ);
    BspScreen_DrawState(cr, mpu.hasData, mpu.stale, mpu.health);
}

/*
 * @brief   Desenha a tela atual na superficie.
 * @retval  void
 */
static void BspScreen_Render(void)
{
    cairo_t *cr = cairo_create(bspScreen.surface);

    BspScreen_SetColor(cr, &colorBackground);
    cairo_paint(cr);

    switch(bspScreen.screen)
    {
        case eSCREEN_MENU:
            BspScreen_DrawMenu(cr);
            break;

        case eSCREEN_MPU:
            BspScreen_DrawMpu(cr);
            break;

        case eSCREEN_BMP:
            BspScreen_DrawBmp(cr);
            break;

        default:
            break;
    }

    cairo_destroy(cr);
}

/*
 * @brief   Redesenha a interface e atualiza o display e a janela.
 * @retval  void
 */
static void BspScreen_Update(void)
{
    BspScreen_Render();
    BspScreen_PanelFlush();

    if(bspScreen.drawingArea != NULL)
    {
        gtk_widget_queue_draw(bspScreen.drawingArea);
    }
}

/*
 * @brief   Aplica um evento de botao na navegacao.
 *          Tela inicial: SELECIONAR alterna MPU/BMP e ENTER abre a tela.
 *          Telas dos sensores: VOLTAR esta sempre selecionado, entao
 *          ENTER volta para a tela inicial.
 * @param   event - Evento recebido
 * @retval  void
 */
static void BspScreen_HandleEvent(bspButtonEvent_t event)
{
    if(bspScreen.screen == eSCREEN_MENU)
    {
        if(event == eBSP_BUTTON_EVENT_SELECT)
        {
            bspScreen.selected = (menuButton_t)(((uint8_t)bspScreen.selected + 1U)
                                                % (uint8_t)eMENU_BUTTON_COUNT);
        }
        else if(event == eBSP_BUTTON_EVENT_ENTER)
        {
            bspScreen.screen = (bspScreen.selected == eMENU_BUTTON_MPU)
                               ? eSCREEN_MPU : eSCREEN_BMP;
        }
    }
    else if(event == eBSP_BUTTON_EVENT_ENTER)
    {
        bspScreen.screen = eSCREEN_MENU;
    }
}

/*
 * @brief   Tarefa periodica: le os botoes fisicos e trata os eventos.
 * @param   userData - Nao utilizado
 * @retval  G_SOURCE_CONTINUE para manter a tarefa
 */
static gboolean BspScreen_OnPollTick(gpointer userData)
{
    bspButtonEvent_t event;
    bool changed = false;

    (void)userData;

    BspButton_Poll();
    event = BspButton_GetEvent();

    while(event != eBSP_BUTTON_EVENT_NONE)
    {
        BspScreen_HandleEvent(event);
        changed = true;
        event = BspButton_GetEvent();
    }

    if(changed == true)
    {
        BspScreen_Update();
    }

    return G_SOURCE_CONTINUE;
}

/*
 * @brief   Tarefa periodica: atualiza os dados mostrados na tela.
 * @param   userData - Nao utilizado
 * @retval  G_SOURCE_CONTINUE para manter a tarefa
 */
static gboolean BspScreen_OnRefreshTick(gpointer userData)
{
    (void)userData;

    BspScreen_Update();

    return G_SOURCE_CONTINUE;
}

/*
 * @brief   Teclado de teste: Espaco = SELECIONAR, Enter = ENTER.
 * @param   controller - Controlador de teclado (nao utilizado)
 * @param   keyval     - Tecla pressionada
 * @param   keycode    - Codigo da tecla (nao utilizado)
 * @param   state      - Modificadores (nao utilizado)
 * @param   userData   - Nao utilizado
 * @retval  TRUE se a tecla foi tratada
 */
static gboolean BspScreen_OnKeyPressed(GtkEventControllerKey *controller,
                                       guint keyval,
                                       guint keycode,
                                       GdkModifierType state,
                                       gpointer userData)
{
    gboolean handled = FALSE;

    (void)controller;
    (void)keycode;
    (void)state;
    (void)userData;

    if(keyval == GDK_KEY_space)
    {
        BspScreen_HandleEvent(eBSP_BUTTON_EVENT_SELECT);
        handled = TRUE;
    }
    else if((keyval == GDK_KEY_Return) || (keyval == GDK_KEY_KP_Enter))
    {
        BspScreen_HandleEvent(eBSP_BUTTON_EVENT_ENTER);
        handled = TRUE;
    }

    if(handled == TRUE)
    {
        BspScreen_Update();
    }

    return handled;
}

/*
 * @brief   Funcao de desenho da janela: copia a superficie do display,
 *          mantendo a proporcao (importante em tela cheia).
 * @param   area     - Area de desenho (nao utilizada)
 * @param   cr       - Contexto Cairo da janela
 * @param   width    - Largura disponivel [pixels]
 * @param   height   - Altura disponivel [pixels]
 * @param   userData - Nao utilizado
 * @retval  void
 */
static void BspScreen_GtkDraw(GtkDrawingArea *area,
                              cairo_t *cr,
                              int width,
                              int height,
                              gpointer userData)
{
    const double scaleX = (double)width / (double)dSCREEN_WIDTH;
    const double scaleY = (double)height / (double)dSCREEN_HEIGHT;
    const double scale = (scaleX < scaleY) ? scaleX : scaleY;

    (void)area;
    (void)userData;

    cairo_set_source_rgb(cr, 0.0, 0.0, 0.0);
    cairo_paint(cr);

    cairo_translate(cr, ((double)width - ((double)dSCREEN_WIDTH * scale)) / 2.0,
                    ((double)height - ((double)dSCREEN_HEIGHT * scale)) / 2.0);
    cairo_scale(cr, scale, scale);
    cairo_set_source_surface(cr, bspScreen.surface, 0.0, 0.0);
    cairo_paint(cr);
}

/*
 * @brief   Fechar a janela encerra o loop de eventos.
 * @param   window   - Janela (nao utilizada)
 * @param   userData - Nao utilizado
 * @retval  FALSE para permitir o fechamento
 */
static gboolean BspScreen_OnCloseRequest(GtkWindow *window, gpointer userData)
{
    (void)window;
    (void)userData;

    g_main_loop_quit(bspScreen.loop);

    return FALSE;
}

/*
 * @brief   SIGINT/SIGTERM encerram o loop, para liberar os GPIOs com
 *          BspGpio_Deinit na Main.
 * @param   userData - Nao utilizado
 * @retval  G_SOURCE_REMOVE
 */
static gboolean BspScreen_OnSignal(gpointer userData)
{
    (void)userData;

    g_main_loop_quit(bspScreen.loop);

    return G_SOURCE_REMOVE;
}

/*
 * @brief   Cria a janela GTK (espelho do display e teclado de teste).
 *          Sem sessao grafica a janela nao existe e o display continua
 *          funcionando normalmente.
 * @retval  void
 */
static void BspScreen_CreateWindow(void)
{
    GtkEventController *keyController;

    if(gtk_init_check() == FALSE)
    {
        (void)fprintf(stderr, "BspScreen: sem sessao grafica, janela desativada\n");
        return;
    }

    bspScreen.window = gtk_window_new();
    bspScreen.drawingArea = gtk_drawing_area_new();

    gtk_window_set_title(GTK_WINDOW(bspScreen.window), dSCREEN_WINDOW_TITLE);
    gtk_window_set_resizable(GTK_WINDOW(bspScreen.window), FALSE);
    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(bspScreen.drawingArea),
                                       dSCREEN_WIDTH);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(bspScreen.drawingArea),
                                        dSCREEN_HEIGHT);
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(bspScreen.drawingArea),
                                   BspScreen_GtkDraw, NULL, NULL);
    gtk_window_set_child(GTK_WINDOW(bspScreen.window), bspScreen.drawingArea);

    keyController = gtk_event_controller_key_new();
    g_signal_connect(keyController, "key-pressed", G_CALLBACK(BspScreen_OnKeyPressed), NULL);
    gtk_widget_add_controller(bspScreen.window, keyController);

    g_signal_connect(bspScreen.window, "close-request",
                     G_CALLBACK(BspScreen_OnCloseRequest), NULL);

#if dSCREEN_FULLSCREEN == 1
    gtk_window_fullscreen(GTK_WINDOW(bspScreen.window));
#endif

    gtk_window_present(GTK_WINDOW(bspScreen.window));
}

int BspScreen_Run(int argc, char **argv)
{
    int status = 1;

    (void)argc;
    (void)argv;

    bspScreen.surface = cairo_image_surface_create(CAIRO_FORMAT_RGB16_565,
                                                   dSCREEN_WIDTH, dSCREEN_HEIGHT);
    bspScreen.loop = g_main_loop_new(NULL, FALSE);

    if((cairo_surface_status(bspScreen.surface) == CAIRO_STATUS_SUCCESS)
       && (bspScreen.loop != NULL))
    {
        bspScreen.screen = eSCREEN_MENU;
        bspScreen.selected = eMENU_BUTTON_MPU;

        BspScreen_PanelInit();
        BspScreen_CreateWindow();

        (void)g_timeout_add(dBSP_BUTTON_POLL_PERIOD_MS, BspScreen_OnPollTick, NULL);
        (void)g_timeout_add(dSCREEN_REFRESH_MS, BspScreen_OnRefreshTick, NULL);
        (void)g_unix_signal_add(SIGINT, BspScreen_OnSignal, NULL);
        (void)g_unix_signal_add(SIGTERM, BspScreen_OnSignal, NULL);

        BspScreen_Update();
        g_main_loop_run(bspScreen.loop);

        status = 0;
    }

    if(bspScreen.window != NULL)
    {
        gtk_window_destroy(GTK_WINDOW(bspScreen.window));
        bspScreen.window = NULL;
        bspScreen.drawingArea = NULL;
    }

    if(bspScreen.loop != NULL)
    {
        g_main_loop_unref(bspScreen.loop);
        bspScreen.loop = NULL;
    }

    cairo_surface_destroy(bspScreen.surface);
    bspScreen.surface = NULL;

    return status;
}