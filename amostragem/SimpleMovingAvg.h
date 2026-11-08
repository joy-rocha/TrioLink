/**************************************
* @file SimpleMovingAvg.h
* @addtogroup SimpleMovingAvg
* @{
***************************************/
#ifndef _SIMPLE_MOVING_AVG_H_
#define _SIMPLE_MOVING_AVG_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include "AssertTypes.h"

/**************************************
* CONFIGURACOES
***************************************/
/** @addtogroup simpleMovingAvg_appCfg Configuracoes da aplicacao.
* @brief Define as constantes da media movel simples.
* @{
***************************************/
/// Numero de amostras da janela da media movel.
#define dSIMPLE_MOVING_AVG_WINDOW_SIZE 15 // [amostras] 1 a 255
/** @} */

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Instancia de uma media movel simples
typedef struct
{
    /// Janela circular com as ultimas amostras
    float buffer[dSIMPLE_MOVING_AVG_WINDOW_SIZE];
    /// Soma das amostras validas da janela
    float sum;
    /// Posicao da proxima escrita (que tambem e a da amostra mais antiga)
    u8 index;
    /// Quantidade de amostras validas (ate dSIMPLE_MOVING_AVG_WINDOW_SIZE)
    u8 count;
} simpleMovingAvg_t;

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
void SimpleMovingAvg_Init(simpleMovingAvg_t *avg);
void SimpleMovingAvg_NewSample(simpleMovingAvg_t *avg, float sample);
float SimpleMovingAvg_GetValue(const simpleMovingAvg_t *avg);
bool SimpleMovingAvg_IsBufferFull(const simpleMovingAvg_t *avg);

#endif /* _SIMPLE_MOVING_AVG_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
