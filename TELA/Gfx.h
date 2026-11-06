/**************************************
* @file Gfx.h
* @addtogroup Gfx
* @brief Primitivas graficas e renderizacao de texto TrueType em framebuffer RGB565.
* @{
***************************************/

#ifndef _GFX_H_
#define _GFX_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include "Lcd.h"
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
/// Alinhamento do texto horizontal
typedef enum
{
    eGFX_ALIGN_LEFT = 0,
    eGFX_ALIGN_CENTER,
    eGFX_ALIGN_RIGHT
} gfxAlign_t;

/**************************************
* VARIAVEIS PUBLICAS
***************************************/
/// Buffer global de imagem RGB565
extern uint16_t frameBuffer[dLCD_W * dLCD_H];

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
/**************************************
/** @brief Converte componentes R, G, B de 8 bits para o formato RGB565 de 16 bits.
* @param red: intensidade da cor vermelha (0-255).
* @param green: intensidade da cor verde (0-255).
* @param blue: intensidade da cor azul (0-255).
* @retval Valor da cor em formato RGB565 (16 bits).
***************************************/
uint16_t Gfx_Rgb(uint8_t red, uint8_t green, uint8_t blue);

/**************************************
/** @brief Inicializa a biblioteca FreeType e carrega a fonte especificada.
* @param fontPath: caminho do ficheiro de fonte TrueType (.ttf).
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Gfx_Init(const char *fontPath);

/**************************************
/** @brief Preenche todo o framebuffer com uma cor solida.
* @param color: cor de preenchimento em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_Clear(uint16_t color);

/**************************************
/** @brief Desenha um retangulo preenchido.
* @param posX: coordenada X inicial.
* @param posY: coordenada Y inicial.
* @param width: largura do retangulo.
* @param height: altura do retangulo.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_FillRect(int posX, int posY, int width, int height, uint16_t color);

/**************************************
/** @brief Desenha um retangulo preenchido com cantos arredondados.
* @param posX: coordenada X inicial.
* @param posY: coordenada Y inicial.
* @param width: largura do retangulo.
* @param height: altura do retangulo.
* @param radius: raio dos cantos arredondados.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_FillRRect(int posX, int posY, int width, int height, int radius, uint16_t color);

/**************************************
/** @brief Desenha um circulo preenchido.
* @param centerX: coordenada X do centro.
* @param centerY: coordenada Y do centro.
* @param radius: raio do circulo.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_FillCircle(int centerX, int centerY, int radius, uint16_t color);

/**************************************
/** @brief Desenha um anel/elipse com espessura especificada.
* @param centerX: coordenada X do centro.
* @param centerY: coordenada Y do centro.
* @param radiusX: raio horizontal.
* @param radiusY: raio vertical.
* @param thickness: espessura do anel.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_Ring(int centerX, int centerY, int radiusX, int radiusY, int thickness, uint16_t color);

/**************************************
/** @brief Renderiza uma string UTF-8 na posicao especificada.
* @param posX: coordenada X de ancora.
* @param posY: coordenada Y do centro vertical do texto.
* @param pixelSize: altura da fonte em pixels.
* @param utf8Text: ponteiro para a string codificada em UTF-8.
* @param color: cor do texto em formato RGB565.
* @param align: alinhamento horizontal do texto.
* @retval Nenhum.
***************************************/
void Gfx_Text(int posX, int posY, int pixelSize, const char *utf8Text, uint16_t color, gfxAlign_t align);

/**************************************
/** @brief Calcula a largura total em pixels de uma string UTF-8.
* @param utf8Text: ponteiro para a string codificada em UTF-8.
* @param pixelSize: altura da fonte em pixels.
* @retval Largura da string em pixels.
***************************************/
int Gfx_TextWidth(const char *utf8Text, int pixelSize);

#endif /* _GFX_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */