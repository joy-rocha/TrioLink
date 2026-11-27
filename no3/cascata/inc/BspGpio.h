/*
 * @file    BspGpio.h
 * @brief   Abstracao de GPIO (libgpiod v2) para o projeto Cascata de Sensores.
 *          Configura todos os pinos do display ILI9341 (barramento paralelo
 *          de 8 bits) e dos botoes de ENTER e SELECAO.
 */

#ifndef BSP_GPIO_H
#define BSP_GPIO_H

#include <stdbool.h>
#include <stdint.h>

/// Largura do barramento de dados do display [bits]
#define dBSP_GPIO_DATA_BUS_WIDTH    8

/// Pinos logicos do projeto (a ordem define o indice interno da tabela)
typedef enum
{
    eBSP_GPIO_LCD_D0 = 0,
    eBSP_GPIO_LCD_D1,
    eBSP_GPIO_LCD_D2,
    eBSP_GPIO_LCD_D3,
    eBSP_GPIO_LCD_D4,
    eBSP_GPIO_LCD_D5,
    eBSP_GPIO_LCD_D6,
    eBSP_GPIO_LCD_D7,
    eBSP_GPIO_LCD_RST,
    eBSP_GPIO_LCD_CS,
    eBSP_GPIO_LCD_RS,
    eBSP_GPIO_LCD_WR,
    eBSP_GPIO_LCD_RD,
    eBSP_GPIO_BTN_ENTER,
    eBSP_GPIO_BTN_SELECT,
    eBSP_GPIO_COUNT
} bspGpioPin_t;

/*
 * @brief   Abre o gpiochip e configura todos os pinos (saidas do display
 *          e entradas com pull-up dos botoes).
 * @param   void
 * @retval  true  - Inicializacao concluida
 * @retval  false - Falha ao abrir o chip ou requisitar as linhas
 */
bool BspGpio_Init(void);

/*
 * @brief   Libera as linhas requisitadas e fecha o gpiochip.
 * @param   void
 * @retval  void
 */
void BspGpio_Deinit(void);

/*
 * @brief   Escreve um nivel logico em um pino de saida.
 * @param   pin   - Pino logico (apenas pinos do display)
 * @param   level - true = nivel alto, false = nivel baixo
 * @retval  true  - Escrita realizada
 * @retval  false - Pino invalido, nao inicializado ou erro na escrita
 */
bool BspGpio_Write(bspGpioPin_t pin, bool level);

/*
 * @brief   Le o nivel logico de um pino de entrada.
 * @param   pin   - Pino logico (apenas botoes)
 * @param   level - Ponteiro que recebe o nivel lido (true = alto)
 * @retval  true  - Leitura realizada
 * @retval  false - Pino invalido, ponteiro nulo ou erro na leitura
 */
bool BspGpio_Read(bspGpioPin_t pin, bool *level);

/*
 * @brief   Escreve um byte no barramento de dados D0..D7 do display em
 *          uma unica chamada.
 * @param   value - Byte a ser colocado no barramento (D0 = bit menos significativo)
 * @retval  true  - Escrita realizada
 * @retval  false - Nao inicializado ou erro na escrita
 */
bool BspGpio_WriteDataBus(uint8_t value);

#endif /* BSP_GPIO_H */