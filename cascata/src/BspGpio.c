/*
 * @file    BspGpio.c
 * @brief   Implementacao da abstracao de GPIO usando libgpiod v2.
 */

#include "BspGpio.h"

#include <gpiod.h>
#include <stddef.h>

/// Caminho do chip de GPIO do header de 40 pinos (RP1 no Raspberry Pi 5)
#define dGPIO_CHIP_PATH         "/dev/gpiochip0"

/// Nome do consumidor exibido no gpioinfo
#define dGPIO_CONSUMER_NAME     "cascata"

/// Linhas do barramento de dados do display
#define dGPIO_LCD_D0_LINE       16
#define dGPIO_LCD_D1_LINE       17
#define dGPIO_LCD_D2_LINE       18
#define dGPIO_LCD_D3_LINE       19
#define dGPIO_LCD_D4_LINE       20
#define dGPIO_LCD_D5_LINE       21
#define dGPIO_LCD_D6_LINE       22
#define dGPIO_LCD_D7_LINE       23

/// Linhas de controle do display
#define dGPIO_LCD_RST_LINE      27
#define dGPIO_LCD_CS_LINE       5
#define dGPIO_LCD_RS_LINE       24
#define dGPIO_LCD_WR_LINE       25
#define dGPIO_LCD_RD_LINE       26

/// Linhas dos botoes
#define dGPIO_BTN_ENTER_LINE    4
#define dGPIO_BTN_SELECT_LINE   6

/// Quantidade de pinos de saida (display) e de entrada (botoes)
#define dGPIO_OUTPUT_COUNT      13
#define dGPIO_INPUT_COUNT       2

/// Tabela de linhas fisicas, indexada por bspGpioPin_t (saidas primeiro)
static const unsigned int pinLines[eBSP_GPIO_COUNT] =
{
    dGPIO_LCD_D0_LINE,
    dGPIO_LCD_D1_LINE,
    dGPIO_LCD_D2_LINE,
    dGPIO_LCD_D3_LINE,
    dGPIO_LCD_D4_LINE,
    dGPIO_LCD_D5_LINE,
    dGPIO_LCD_D6_LINE,
    dGPIO_LCD_D7_LINE,
    dGPIO_LCD_RST_LINE,
    dGPIO_LCD_CS_LINE,
    dGPIO_LCD_RS_LINE,
    dGPIO_LCD_WR_LINE,
    dGPIO_LCD_RD_LINE,
    dGPIO_BTN_ENTER_LINE,
    dGPIO_BTN_SELECT_LINE
};

/// Dados locais do modulo
static struct
{
    struct gpiod_chip *chip;
    struct gpiod_line_request *outputRequest;
    struct gpiod_line_request *inputRequest;
    bool initialized;
} bspGpio;

/*
 * @brief   Requisita um grupo de linhas ao kernel como saida ou entrada.
 * @param   lines    - Vetor com os numeros das linhas
 * @param   count    - Quantidade de linhas no vetor
 * @param   isOutput - true = saida (nivel baixo inicial), false = entrada com pull-up
 * @retval  Ponteiro da requisicao, ou NULL em caso de falha
 */
static struct gpiod_line_request *BspGpio_RequestLines(const unsigned int *lines,
                                                       size_t count,
                                                       bool isOutput)
{
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *lineConfig = gpiod_line_config_new();
    struct gpiod_request_config *requestConfig = gpiod_request_config_new();
    struct gpiod_line_request *request = NULL;

    if((settings != NULL) && (lineConfig != NULL) && (requestConfig != NULL))
    {
        if(isOutput == true)
        {
            gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
            gpiod_line_settings_set_output_value(settings, GPIOD_LINE_VALUE_INACTIVE);
        }
        else
        {
            gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_INPUT);
            gpiod_line_settings_set_bias(settings, GPIOD_LINE_BIAS_PULL_UP);
        }

        if(gpiod_line_config_add_line_settings(lineConfig, lines, count, settings) == 0)
        {
            gpiod_request_config_set_consumer(requestConfig, dGPIO_CONSUMER_NAME);
            request = gpiod_chip_request_lines(bspGpio.chip, requestConfig, lineConfig);
        }
    }

    gpiod_request_config_free(requestConfig);
    gpiod_line_config_free(lineConfig);
    gpiod_line_settings_free(settings);

    return request;
}

bool BspGpio_Init(void)
{
    bool result = false;

    if(bspGpio.initialized == true)
    {
        return true;
    }

    bspGpio.chip = gpiod_chip_open(dGPIO_CHIP_PATH);

    if(bspGpio.chip != NULL)
    {
        bspGpio.outputRequest = BspGpio_RequestLines(&pinLines[0],
                                                     dGPIO_OUTPUT_COUNT,
                                                     true);
        bspGpio.inputRequest = BspGpio_RequestLines(&pinLines[dGPIO_OUTPUT_COUNT],
                                                    dGPIO_INPUT_COUNT,
                                                    false);

        if((bspGpio.outputRequest != NULL) && (bspGpio.inputRequest != NULL))
        {
            bspGpio.initialized = true;
            result = true;
        }
        else
        {
            BspGpio_Deinit();
        }
    }

    return result;
}

void BspGpio_Deinit(void)
{
    if(bspGpio.outputRequest != NULL)
    {
        gpiod_line_request_release(bspGpio.outputRequest);
        bspGpio.outputRequest = NULL;
    }

    if(bspGpio.inputRequest != NULL)
    {
        gpiod_line_request_release(bspGpio.inputRequest);
        bspGpio.inputRequest = NULL;
    }

    if(bspGpio.chip != NULL)
    {
        gpiod_chip_close(bspGpio.chip);
        bspGpio.chip = NULL;
    }

    bspGpio.initialized = false;
}

bool BspGpio_Write(bspGpioPin_t pin, bool level)
{
    enum gpiod_line_value value = GPIOD_LINE_VALUE_INACTIVE;

    if((bspGpio.initialized == false) || (pin >= eBSP_GPIO_BTN_ENTER))
    {
        return false;
    }

    if(level == true)
    {
        value = GPIOD_LINE_VALUE_ACTIVE;
    }

    return (gpiod_line_request_set_value(bspGpio.outputRequest,
                                         pinLines[pin],
                                         value) == 0);
}

bool BspGpio_Read(bspGpioPin_t pin, bool *level)
{
    enum gpiod_line_value value;

    if((bspGpio.initialized == false) || (level == NULL)
       || (pin < eBSP_GPIO_BTN_ENTER) || (pin >= eBSP_GPIO_COUNT))
    {
        return false;
    }

    value = gpiod_line_request_get_value(bspGpio.inputRequest, pinLines[pin]);

    if(value == GPIOD_LINE_VALUE_ERROR)
    {
        return false;
    }

    *level = (value == GPIOD_LINE_VALUE_ACTIVE);

    return true;
}

bool BspGpio_WriteDataBus(uint8_t value)
{
    enum gpiod_line_value values[dBSP_GPIO_DATA_BUS_WIDTH];
    uint8_t bit;

    if(bspGpio.initialized == false)
    {
        return false;
    }

    for(bit = 0; bit < dBSP_GPIO_DATA_BUS_WIDTH; bit++)
    {
        if((value & (1U << bit)) != 0U)
        {
            values[bit] = GPIOD_LINE_VALUE_ACTIVE;
        }
        else
        {
            values[bit] = GPIOD_LINE_VALUE_INACTIVE;
        }
    }

    return (gpiod_line_request_set_values_subset(bspGpio.outputRequest,
                                                 dBSP_GPIO_DATA_BUS_WIDTH,
                                                 &pinLines[eBSP_GPIO_LCD_D0],
                                                 values) == 0);
}