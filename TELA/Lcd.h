/**************************************
* @file Lcd.h
* @addtogroup Lcd
* @brief Driver do ecra LCD ILI9341 e controlador de toque.
* @{
***************************************/

#ifndef _LCD_H_
#define _LCD_H_

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
/// Largura do display em pixels
#define dLCD_W 320
/// Altura do display em pixels
#define dLCD_H 240

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
/**************************************
/** @brief Inicializa o display ILI9341 e tenta ativar o modo rapido.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Lcd_Init(void);

/**************************************
/** @brief Encerra a interface do display e desmapeia a memoria RIO.
* @param Nenhum.
* @retval Nenhum.
***************************************/
void Lcd_Close(void);

/**************************************
/** @brief Envia apenas a regiao alterada do framebuffer para o display.
* @param frameBuffer: ponteiro para o buffer de imagem RGB565.
* @retval Nenhum.
***************************************/
void Lcd_Flush(const uint16_t *frameBuffer);

/**************************************
/** @brief Inicializa o controlador de toque XPT2046.
* @param Nenhum.
* @retval 0 em caso de sucesso.
***************************************/
int Touch_Init(void);

/**************************************
/** @brief Realiza a leitura dos valores brutos do sensor de toque.
* @param rawX: ponteiro para armazenar a leitura bruta X.
* @param rawY: ponteiro para armazenar a leitura bruta Y.
* @retval true se houver toque, false caso contrario.
***************************************/
bool Touch_ReadRaw(int *rawX, int *rawY);

/**************************************
/** @brief Realiza a leitura das coordenadas convertidas em pixels.
* @param pixelX: ponteiro para armazenar a coordenada X em pixels.
* @param pixelY: ponteiro para armazenar a coordenada Y em pixels.
* @retval true se houver toque, false caso contrario.
***************************************/
bool Touch_Read(int *pixelX, int *pixelY);

#endif /* _LCD_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */