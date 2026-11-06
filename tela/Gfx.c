/**************************************
* @file Gfx.c
* @addtogroup Gfx
* @brief Primitivas graficas e renderizacao de texto TrueType em framebuffer RGB565.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - FreeType2
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 05/11/2026</b> \n Assert \n Primeira versao adaptada.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "Gfx.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <math.h>
#include <stdio.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Mascara para componente vermelho em RGB565
#define dGFX_COLOR_R_MASK      0x1F
/// Mascara para componente verde em RGB565
#define dGFX_COLOR_G_MASK      0x3F
/// Mascara para componente azul em RGB565
#define dGFX_COLOR_B_MASK      0x1F
/// Deslocamento de bits para o componente vermelho
#define dGFX_R_SHIFT           11
/// Deslocamento de bits para o componente verde
#define dGFX_G_SHIFT           5
/// Valor maximo de opacidade alpha
#define dGFX_MAX_ALPHA         255
/// Constante de escala da linha de base da fonte
#define dGFX_BASELINE_FACTOR   36
/// Divisor percentual da linha de base
#define dGFX_BASELINE_DIVISOR  100
/// Deslocamento de bits para conversao de unidades do FreeType
#define dGFX_FT_POS_SHIFT      6

/**************************************
* CONSTANTES
***************************************/

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Variaveis internas do modulo Gfx
static struct
{
    FT_Library ftLibrary;
    FT_Face ftFace;
    int currentPixelSize;
} gfx = {
    .currentPixelSize = 0
};

/// Buffer global de imagem RGB565
uint16_t frameBuffer[dLCD_W * dLCD_H];

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static void Gfx_SetPixelSize(int pixelSize);
static inline void Gfx_PutPixel(int posX, int posY, uint16_t color);
static inline void Gfx_BlendPixel(int posX, int posY, uint16_t color, int alpha);
static int Gfx_Utf8NextChar(const char **stringPtr);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
* @brief Converte componentes R, G, B de 8 bits para o formato RGB565 de 16 bits.
* @param red: intensidade da cor vermelha (0-255).
* @param green: intensidade da cor verde (0-255).
* @param blue: intensidade da cor azul (0-255).
* @retval Valor da cor em formato RGB565 (16 bits).
***************************************/
uint16_t Gfx_Rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    return (uint16_t)((((uint16_t)(red & 0xF8)) << 8) | (((uint16_t)(green & 0xFC)) << 3) | (blue >> 3));
}

/**************************************
* @brief Inicializa a biblioteca FreeType e carrega a fonte especificada.
* @param fontPath: caminho do ficheiro de fonte TrueType (.ttf).
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Gfx_Init(const char *fontPath)
{
    if(FT_Init_FreeType(&gfx.ftLibrary) != 0)
    {
        return -1;
    }

    if(FT_New_Face(gfx.ftLibrary, fontPath, 0, &gfx.ftFace) != 0)
    {
        fprintf(stderr, "Fonte nao encontrada: %s\n", fontPath);
        return -1;
    }

    return 0;
}

/**************************************
* @brief Preenche todo o framebuffer com uma cor solida.
* @param color: cor de preenchimento em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_Clear(uint16_t color)
{
    for(int i = 0; i < (dLCD_W * dLCD_H); i++)
    {
        frameBuffer[i] = color;
    }
}

/**************************************
* @brief Desenha um retangulo preenchido.
* @param posX: coordenada X inicial.
* @param posY: coordenada Y inicial.
* @param width: largura do retangulo.
* @param height: altura do retangulo.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_FillRect(int posX, int posY, int width, int height, uint16_t color)
{
    for(int j = posY; j < (posY + height); j++)
    {
        for(int i = posX; i < (posX + width); i++)
        {
            Gfx_PutPixel(i, j, color);
        }
    }
}

/**************************************
* @brief Desenha um retangulo preenchido com cantos arredondados.
* @param posX: coordenada X inicial.
* @param posY: coordenada Y inicial.
* @param width: largura do retangulo.
* @param height: altura do retangulo.
* @param radius: raio dos cantos arredondados.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_FillRRect(int posX, int posY, int width, int height, int radius, uint16_t color)
{
    if(radius > (width / 2))
    {
        radius = width / 2;
    }

    if(radius > (height / 2))
    {
        radius = height / 2;
    }

    for(int j = 0; j < height; j++)
    {
        int inset = 0;
        float deltaY = -1.0f;

        if(j < radius)
        {
            deltaY = (float)radius - (float)j - 0.5f;
        }
        else if(j >= (height - radius))
        {
            deltaY = (float)j - (float)(height - radius) + 0.5f;
        }

        if(deltaY >= 0.0f)
        {
            inset = (int)ceilf((float)radius - sqrtf(((float)radius * (float)radius) - (deltaY * deltaY)));
        }

        for(int i = inset; i < (width - inset); i++)
        {
            Gfx_PutPixel(posX + i, posY + j, color);
        }
    }
}

/**************************************
* @brief Desenha um circulo preenchido.
* @param centerX: coordenada X do centro.
* @param centerY: coordenada Y do centro.
* @param radius: raio do circulo.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_FillCircle(int centerX, int centerY, int radius, uint16_t color)
{
    for(int j = -radius; j <= radius; j++)
    {
        for(int i = -radius; i <= radius; i++)
        {
            if(((i * i) + (j * j)) <= (radius * radius))
            {
                Gfx_PutPixel(centerX + i, centerY + j, color);
            }
        }
    }
}

/**************************************
* @brief Desenha um anel/elipse com espessura especificada.
* @param centerX: coordenada X do centro.
* @param centerY: coordenada Y do centro.
* @param radiusX: raio horizontal.
* @param radiusY: raio vertical.
* @param thickness: espessura do anel.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
void Gfx_Ring(int centerX, int centerY, int radiusX, int radiusY, int thickness, uint16_t color)
{
    float minRadius = (float)((radiusX < radiusY) ? radiusX : radiusY);

    for(int j = -radiusY - thickness; j <= (radiusY + thickness); j++)
    {
        for(int i = -radiusX - thickness; i <= (radiusX + thickness); i++)
        {
            float dist = hypotf((float)i / (float)radiusX, (float)j / (float)radiusY);
            if((fabsf(dist - 1.0f) * minRadius) <= ((float)thickness / 2.0f))
            {
                Gfx_PutPixel(centerX + i, centerY + j, color);
            }
        }
    }
}

/**************************************
* @brief Calcula a largura total em pixels de uma string UTF-8.
* @param utf8Text: ponteiro para a string codificada em UTF-8.
* @param pixelSize: altura da fonte em pixels.
* @retval Largura da string em pixels.
***************************************/
int Gfx_TextWidth(const char *utf8Text, int pixelSize)
{
    Gfx_SetPixelSize(pixelSize);
    int totalWidth = 0;
    const char *textPtr = utf8Text;

    while(*textPtr != '\0')
    {
        int codePoint = Gfx_Utf8NextChar(&textPtr);
        if(FT_Load_Char(gfx.ftFace, codePoint, FT_LOAD_DEFAULT) != 0)
        {
            continue;
        }

        totalWidth += (int)(gfx.ftFace->glyph->advance.x >> dGFX_FT_POS_SHIFT);
    }

    return totalWidth;
}

/**************************************
* @brief Renderiza uma string UTF-8 na posicao especificada.
* @param posX: coordenada X de ancora.
* @param posY: coordenada Y do centro vertical do texto.
* @param pixelSize: altura da fonte em pixels.
* @param utf8Text: ponteiro para a string codificada em UTF-8.
* @param color: cor do texto em formato RGB565.
* @param align: alinhamento horizontal do texto.
* @retval Nenhum.
***************************************/
void Gfx_Text(int posX, int posY, int pixelSize, const char *utf8Text, uint16_t color, gfxAlign_t align)
{
    Gfx_SetPixelSize(pixelSize);
    int textWidth = Gfx_TextWidth(utf8Text, pixelSize);
    int penPosition = posX;

    if(align == eGFX_ALIGN_CENTER)
    {
        penPosition = posX - (textWidth / 2);
    }
    else if(align == eGFX_ALIGN_RIGHT)
    {
        penPosition = posX - textWidth;
    }

    int baseLine = posY + ((pixelSize * dGFX_BASELINE_FACTOR) / dGFX_BASELINE_DIVISOR);
    const char *textPtr = utf8Text;

    while(*textPtr != '\0')
    {
        int codePoint = Gfx_Utf8NextChar(&textPtr);
        if(FT_Load_Char(gfx.ftFace, codePoint, FT_LOAD_RENDER) != 0)
        {
            continue;
        }

        FT_GlyphSlot glyph = gfx.ftFace->glyph;
        for(unsigned int r = 0; r < glyph->bitmap.rows; r++)
        {
            for(unsigned int c = 0; c < glyph->bitmap.width; c++)
            {
                int alphaVal = glyph->bitmap.buffer[(r * glyph->bitmap.pitch) + c];
                if(alphaVal > 0)
                {
                    Gfx_BlendPixel(penPosition + glyph->bitmap_left + (int)c,
                                   baseLine - glyph->bitmap_top + (int)r,
                                   color,
                                   alphaVal);
                }
            }
        }

        penPosition += (int)(glyph->advance.x >> dGFX_FT_POS_SHIFT);
    }
}

/**************************************
* FUNCOES LOCAIS
***************************************/

/**************************************
* @brief Atualiza o tamanho da fonte no FreeType apenas se for diferente do atual.
* @param pixelSize: tamanho da fonte em pixels.
* @retval Nenhum.
***************************************/
static void Gfx_SetPixelSize(int pixelSize)
{
    if(pixelSize != gfx.currentPixelSize)
    {
        FT_Set_Pixel_Sizes(gfx.ftFace, 0, pixelSize);
        gfx.currentPixelSize = pixelSize;
    }
}

/**************************************
* @brief Escreve um pixel no framebuffer sem mistura alpha.
* @param posX: coordenada X do pixel.
* @param posY: coordenada Y do pixel.
* @param color: cor em formato RGB565.
* @retval Nenhum.
***************************************/
static inline void Gfx_PutPixel(int posX, int posY, uint16_t color)
{
    if(((unsigned int)posX < dLCD_W) && ((unsigned int)posY < dLCD_H))
    {
        frameBuffer[(posY * dLCD_W) + posX] = color;
    }
}

/**************************************
* @brief Mistura a cor do pixel com a cor existente usando canal Alpha.
* @param posX: coordenada X do pixel.
* @param posY: coordenada Y do pixel.
* @param color: cor fonte em formato RGB565.
* @param alpha: valor alpha (0-255).
* @retval Nenhum.
***************************************/
static inline void Gfx_BlendPixel(int posX, int posY, uint16_t color, int alpha)
{
    if(((unsigned int)posX >= dLCD_W) || ((unsigned int)posY >= dLCD_H))
    {
        return;
    }

    uint16_t destColor = frameBuffer[(posY * dLCD_W) + posX];

    int destR = (destColor >> dGFX_R_SHIFT) & dGFX_COLOR_R_MASK;
    int destG = (destColor >> dGFX_G_SHIFT) & dGFX_COLOR_G_MASK;
    int destB = destColor & dGFX_COLOR_B_MASK;

    int srcR = (color >> dGFX_R_SHIFT) & dGFX_COLOR_R_MASK;
    int srcG = (color >> dGFX_G_SHIFT) & dGFX_COLOR_G_MASK;
    int srcB = color & dGFX_COLOR_B_MASK;

    destR = ((destR * (dGFX_MAX_ALPHA - alpha)) + (srcR * alpha)) / dGFX_MAX_ALPHA;
    destG = ((destG * (dGFX_MAX_ALPHA - alpha)) + (srcG * alpha)) / dGFX_MAX_ALPHA;
    destB = ((destB * (dGFX_MAX_ALPHA - alpha)) + (srcB * alpha)) / dGFX_MAX_ALPHA;

    frameBuffer[(posY * dLCD_W) + posX] = (uint16_t)((destR << dGFX_R_SHIFT) | (destG << dGFX_G_SHIFT) | destB);
}

/**************************************
* @brief Decodifica o proximo caractere UTF-8 de uma string.
* @param stringPtr: ponteiro para o ponteiro da string.
* @retval Codepoint do caractere lido.
***************************************/
static int Gfx_Utf8NextChar(const char **stringPtr)
{
    const unsigned char *ptr = (const unsigned char *)*stringPtr;
    int charVal = *ptr;

    if(charVal < 0x80)
    {
        *stringPtr += 1;
        return charVal;
    }

    if(((charVal & 0xE0) == 0xC0) && (ptr[1] != 0))
    {
        *stringPtr += 2;
        return ((charVal & 0x1F) << 6) | (ptr[1] & 0x3F);
    }

    if(((charVal & 0xF0) == 0xE0) && (ptr[1] != 0) && (ptr[2] != 0))
    {
        *stringPtr += 3;
        return ((charVal & 0x0F) << 12) | ((ptr[1] & 0x3F) << 6) | (ptr[2] & 0x3F);
    }

    *stringPtr += 1;
    return '?';
}
/** @} DOXYGEN GROUP TAG END OF FILE */