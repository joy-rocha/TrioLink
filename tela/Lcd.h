/**************************************
* @file Lcd.h
* @addtogroup Lcd
* @brief Driver paralelo de 8 bits para o display ILI9341 e suporte a touch.
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
* @brief Inicializa o controlador do display ILI9341.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Lcd_Init(void);

/**************************************
* @brief Libera os recursos do display e encerra a conexao GPIO/mmap.
* @param Nenhum.
* @retval Nenhum.
***************************************/
void Lcd_Close(void);

/**************************************
* @brief Envia apenas a regiao alterada do framebuffer para o display.
* @param frameBufferPtr: ponteiro para o buffer de imagem RGB565.
* @retval Nenhum.
***************************************/
void Lcd_Flush(const uint16_t *frameBufferPtr);

/**************************************
* @brief Inicializa a interface de touch XPT2046.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Touch_Init(void);

/**************************************
* @brief Le as coordenadas brutas do controlador de touch.
* @param rawX: ponteiro para armazenar o valor X bruto.
* @param rawY: ponteiro para armazenar o valor Y bruto.
* @retval true se houver toque, false caso contrario.
***************************************/
bool Touch_ReadRaw(int *rawX, int *rawY);

/**************************************
* @brief Le as coordenadas convertidas em pixels do touch.
* @param posX: ponteiro para armazenar a coordenada X em pixels.
* @param posY: ponteiro para armazenar a coordenada Y em pixels.
* @retval true se houver toque, false caso contrario.
***************************************/
bool Touch_Read(int *posX, int *posY);

#endif /* _LCD_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */