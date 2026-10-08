/*******************************************************************************************************/
/** @file         AssertTypes.h
*   @addtogroup   AssertTypes
*   @brief        Definicoes auxiliares para configuracao e tratamento de tipos de variaveis
*   @author       Luiz Neto
*   @details
*   \n <b>Ferramentas:</b> generico
*
*   Changelog
*   @version      <b>1.0.0 - 12/05/2026</b> \n Luiz Neto \n Primeira versao.
*
*   @copyright
*   @{
********************************************************************************************************/

#ifndef ASSERTTYPES_H_
#define ASSERTTYPES_H_

/********************************************************************************************************
*   INCLUDES NECESSARIOS
********************************************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*******************************************************************************************************/
/** @addtogroup assertTypes_types Tipos de dados
*   @brief Definicoes para o padrao de typedefs utilizados no Assert.
*   @{
********************************************************************************************************/

typedef int64_t     s64;
typedef int32_t     s32;
typedef int16_t     s16;
typedef int8_t      s8;

typedef uint64_t    u64;
typedef uint32_t    u32;
typedef uint16_t    u16;
typedef uint8_t     u8;

typedef float       f32;

#define dNULL       ((void *)0)
#define dFALSE      (0U)
#define dTRUE       (1U)

/** @} assertTypes_types */

/*******************************************************************************************************/
/** @addtogroup assertTypes_limits Valores maximos de dados
*   @brief Valores uteis para manipulacao algebrica.
*   @{
********************************************************************************************************/

#define dMAX_U8     (255)
#define dMAX_S8     127
#define dMIN_S8     (-127)

#define dMAX_U16    (65535)
#define dMAX_U32    (0xFFFFFFFF)    //4294967295u

/** @} assertTypes_limits */

/*******************************************************************************************************/
/** @addtogroup assertTypes_bitOps Operacoes de manipulacao de bits
 *  @brief Macros uteis para operacoes de bits.
 *  @{
 *******************************************************************************************************/

/// Seta o bit na posicao POS do valor VAL
#define dBIT_SET(VAL, POS)          (VAL |= (1UL << POS))

/// Limpa o bit na posicao POS do valor VAL
#define dBIT_CLEAR(VAL, POS)        (VAL &= ~(1UL << POS))

/// Inverte o bit na posicao POS do valor VAL
#define dBIT_TOGGLE(VAL, POS)       (VAL ^= (1UL << POS))

/// Escreve o valor BIT (0 ou 1) no bit da posicao POS do valor VAL
#define dBIT_WRITE(VAL, POS, BIT)   (VAL = (VAL & ~(1UL << POS)) | ((BIT & 1UL) << POS))

/// Retorna o valor (0 ou 1) do bit na posicao POS do valor VAL
#define dBIT_READ(VAL, POS)         ((VAL >> POS) & 1UL)

/** @} assertTypes_bitOps */

#endif /* ASSERTTYPES_H_ */

/** @} FILE */
