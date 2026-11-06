/**************************************
* @file Lcd.c
* @addtogroup Lcd
* @brief Driver Paralelo de 8 bits ILI9341 para Raspberry Pi 5.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - lgpio
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 05/11/2026</b> \n Assert \n Primeira versao adaptada.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "Lcd.h"
#include <fcntl.h>
#include <lgpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Identificador do chip GPIO
#define dGPIO_CHIP 0

/// Pino de controle DC (Command/Data)
#define dPIN_DC 24
/// Pino de controle Reset
#define dPIN_RST 25
/// Pino de controle Chip Select
#define dPIN_CS 8
/// Pino de controle Write
#define dPIN_WR 23

/// Mascara do grupo de pinos lgpio (bits 0 a 8)
#define dGRP_MASK 0x1FFULL
/// Bit referente ao sinal WR no grupo lgpio
#define dGRP_WR 0x100ULL

/// Dispositivo de mapeamento de memoria do RP1
#define dGPIOMEM_DEV "/dev/gpiomem0"
/// Tamanho da regiao de memoria mapeada
#define dGPIOMEM_SIZE 0x30000
/// Deslocamento do registrador RIO bank0
#define dRIO_OFFSET 0x10000

/// Quantidade de valores para a tabela LUT
#define dLUT_SIZE 256
/// Tamanho total de pinos no barramento de dados
#define dDATA_PINS_COUNT 8
/// Tamanho do grupo de pinos lgpio
#define dGROUP_PINS_COUNT 9

/**************************************
* CONSTANTES
***************************************/
/// Mapeamento dos pinos de dados D0 ate D7
static const int pinsData[dDATA_PINS_COUNT] = { 12, 13, 16, 19, 20, 21, 26, 27 };

/// Mapeamento do grupo de pinos para lgpio
static const int groupPins[dGROUP_PINS_COUNT] = { 12, 13, 16, 19, 20, 21, 26, 27, dPIN_WR };

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Variaveis internas do driver LCD
static struct
{
    int gpioHandle;
    int groupHandle;
    volatile uint32_t *rio;
    void *mapBase;
    uint32_t lut[dLUT_SIZE];
    uint32_t allMask;
    uint16_t prevFrame[dLCD_W * dLCD_H];
    bool havePrevFrame;
} lcd = {
    .gpioHandle = -1,
    .groupHandle = -1,
    .rio = NULL,
    .mapBase = NULL,
    .havePrevFrame = false
};

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static inline void Lcd_Write8(uint8_t data);
static void Lcd_Cmd(uint8_t command);
static void Lcd_DataByte(uint8_t data);
static void Lcd_Cmdv(uint8_t command, const uint8_t *args, int count);
static bool Lcd_FastInit(void);
static void Lcd_PushPixels(const uint16_t *frameBuffer, int x0, int y0, int x1, int y1);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
/** @brief Inicializa o display ILI9341 e tenta ativar o modo rapido.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Lcd_Init(void)
{
    lcd.gpioHandle = lgGpiochipOpen(dGPIO_CHIP);
    if(lcd.gpioHandle < 0)
    {
        fprintf(stderr, "lgGpiochipOpen falhou (%d)\n", lcd.gpioHandle);
        return -1;
    }

    lgGpioClaimOutput(lcd.gpioHandle, 0, dPIN_DC, 0);
    lgGpioClaimOutput(lcd.gpioHandle, 0, dPIN_RST, 1);
    lgGpioClaimOutput(lcd.gpioHandle, 0, dPIN_CS, 1);

    const int initialLevels[dGROUP_PINS_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0, 1 };
    if(lgGroupClaimOutput(lcd.gpioHandle, 0, dGROUP_PINS_COUNT, groupPins, initialLevels) < 0)
    {
        fprintf(stderr, "lgGroupClaimOutput falhou\n");
        return -1;
    }
    lcd.groupHandle = groupPins[0];

    if(Lcd_FastInit() == true)
    {
        fprintf(stderr, "LCD: modo rapido (registradores RP1)\n");
    }
    else
    {
        fprintf(stderr, "LCD: modo lgpio (sem acesso a %s)\n", dGPIOMEM_DEV);
    }

    /* Reset por Hardware */
    lgGpioWrite(lcd.gpioHandle, dPIN_RST, 1);
    usleep(10000);
    lgGpioWrite(lcd.gpioHandle, dPIN_RST, 0);
    usleep(20000);
    lgGpioWrite(lcd.gpioHandle, dPIN_RST, 1);
    usleep(120000);

    /* Inicializacao do Controlador ILI9341 */
    const uint8_t arg1[] = { 0x03, 0x80, 0x02 };
    Lcd_Cmdv(0xEF, arg1, 3);

    const uint8_t arg2[] = { 0x00, 0xC1, 0x30 };
    Lcd_Cmdv(0xCF, arg2, 3);

    const uint8_t arg3[] = { 0x64, 0x03, 0x12, 0x81 };
    Lcd_Cmdv(0xED, arg3, 4);

    const uint8_t arg4[] = { 0x85, 0x00, 0x78 };
    Lcd_Cmdv(0xE8, arg4, 3);

    const uint8_t arg5[] = { 0x39, 0x2C, 0x00, 0x34, 0x02 };
    Lcd_Cmdv(0xCB, arg5, 5);

    const uint8_t arg6[] = { 0x20 };
    Lcd_Cmdv(0xF7, arg6, 1);

    const uint8_t arg7[] = { 0x00, 0x00 };
    Lcd_Cmdv(0xEA, arg7, 2);

    const uint8_t arg8[] = { 0x23 };
    Lcd_Cmdv(0xC0, arg8, 1);

    const uint8_t arg9[] = { 0x10 };
    Lcd_Cmdv(0xC1, arg9, 1);

    const uint8_t arg10[] = { 0x3E, 0x28 };
    Lcd_Cmdv(0xC5, arg10, 2);

    const uint8_t arg11[] = { 0x86 };
    Lcd_Cmdv(0xC7, arg11, 1);

    const uint8_t arg12[] = { 0x28 };
    Lcd_Cmdv(0x36, arg12, 1);

    const uint8_t arg13[] = { 0x55 };
    Lcd_Cmdv(0x3A, arg13, 1);

    const uint8_t arg14[] = { 0x00, 0x18 };
    Lcd_Cmdv(0xB1, arg14, 2);

    const uint8_t arg15[] = { 0x08, 0x82, 0x27 };
    Lcd_Cmdv(0xB6, arg15, 3);

    const uint8_t arg16[] = { 0x00 };
    Lcd_Cmdv(0xF2, arg16, 1);

    const uint8_t arg17[] = { 0x01 };
    Lcd_Cmdv(0x26, arg17, 1);

    const uint8_t arg18[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
    Lcd_Cmdv(0xE0, arg18, 15);

    const uint8_t arg19[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
    Lcd_Cmdv(0xE1, arg19, 15);

    Lcd_Cmd(0x11);
    usleep(120000);

    Lcd_Cmd(0x29);
    usleep(20000);

    lcd.havePrevFrame = false;
    return 0;
}

/**************************************
/** @brief Envia apenas a regiao alterada do framebuffer para o display.
* @param frameBuffer: ponteiro para o buffer de imagem RGB565.
* @retval Nenhum.
***************************************/
void Lcd_Flush(const uint16_t *frameBuffer)
{
    int x0 = 0;
    int y0 = 0;
    int x1 = dLCD_W - 1;
    int y1 = dLCD_H - 1;

    if(lcd.havePrevFrame == true)
    {
        x0 = dLCD_W;
        y0 = dLCD_H;
        x1 = -1;
        y1 = -1;

        for(int y = 0; y < dLCD_H; y++)
        {
            const uint16_t *currRow = frameBuffer + (y * dLCD_W);
            const uint16_t *prevRow = lcd.prevFrame + (y * dLCD_W);

            if(memcmp(currRow, prevRow, dLCD_W * sizeof(uint16_t)) == 0)
            {
                continue;
            }

            if(y < y0)
            {
                y0 = y;
            }
            y1 = y;

            for(int x = 0; x < dLCD_W; x++)
            {
                if(currRow[x] != prevRow[x])
                {
                    if(x < x0)
                    {
                        x0 = x;
                    }
                    break;
                }
            }

            for(int x = dLCD_W - 1; x >= 0; x--)
            {
                if(currRow[x] != prevRow[x])
                {
                    if(x > x1)
                    {
                        x1 = x;
                    }
                    break;
                }
            }
        }

        if(x1 < 0)
        {
            return;
        }
    }

    memcpy(lcd.prevFrame, frameBuffer, sizeof(lcd.prevFrame));
    lcd.havePrevFrame = true;

    uint8_t windowArgs[4];

    windowArgs[0] = (uint8_t)(x0 >> 8);
    windowArgs[1] = (uint8_t)(x0 & 0xFF);
    windowArgs[2] = (uint8_t)(x1 >> 8);
    windowArgs[3] = (uint8_t)(x1 & 0xFF);
    Lcd_Cmdv(0x2A, windowArgs, 4);

    windowArgs[0] = (uint8_t)(y0 >> 8);
    windowArgs[1] = (uint8_t)(y0 & 0xFF);
    windowArgs[2] = (uint8_t)(y1 >> 8);
    windowArgs[3] = (uint8_t)(y1 & 0xFF);
    Lcd_Cmdv(0x2B, windowArgs, 4);

    Lcd_Cmd(0x2C);

    Lcd_PushPixels(frameBuffer, x0, y0, x1, y1);
}

/**************************************
/** @brief Encerra a interface do display e desmapeia a memoria RIO.
* @param Nenhum.
* @retval Nenhum.
***************************************/
void Lcd_Close(void)
{
    if(lcd.mapBase != NULL)
    {
        munmap(lcd.mapBase, dGPIOMEM_SIZE);
        lcd.mapBase = NULL;
        lcd.rio = NULL;
    }

    if(lcd.gpioHandle >= 0)
    {
        lgGpiochipClose(lcd.gpioHandle);
        lcd.gpioHandle = -1;
    }
    lcd.groupHandle = -1;
}

/**************************************
/** @brief Inicializa o controlador de toque XPT2046.
* @param Nenhum.
* @retval 0 em caso de sucesso.
***************************************/
int Touch_Init(void)
{
    return 0;
}

/**************************************
/** @brief Realiza a leitura dos valores brutos do sensor de toque.
* @param rawX: ponteiro para armazenar a leitura bruta X.
* @param rawY: ponteiro para armazenar a leitura bruta Y.
* @retval true se houver toque, false caso contrario.
***************************************/
bool Touch_ReadRaw(int *rawX, int *rawY)
{
    (void)rawX;
    (void)rawY;
    return false;
}

/**************************************
/** @brief Realiza a leitura das coordenadas convertidas em pixels.
* @param pixelX: ponteiro para armazenar a coordenada X em pixels.
* @param pixelY: ponteiro para armazenar a coordenada Y em pixels.
* @retval true se houver toque, false caso contrario.
***************************************/
bool Touch_Read(int *pixelX, int *pixelY)
{
    (void)pixelX;
    (void)pixelY;
    return false;
}

/**************************************
* FUNCOES LOCAIS
***************************************/

/**************************************
/** @brief Escreve 1 byte no barramento usando lgpio.
* @param data: byte a ser escrito no barramento.
* @retval Nenhum.
***************************************/
static inline void Lcd_Write8(uint8_t data)
{
    lgGroupWrite(lcd.gpioHandle, lcd.groupHandle, (uint64_t)data, dGRP_MASK);
    lgGroupWrite(lcd.gpioHandle, lcd.groupHandle, (uint64_t)data | dGRP_WR, dGRP_MASK);
}

/**************************************
/** @brief Envia um comando para o display LCD.
* @param command: codigo do comando.
* @retval Nenhum.
***************************************/
static void Lcd_Cmd(uint8_t command)
{
    lgGpioWrite(lcd.gpioHandle, dPIN_DC, 0);
    lgGpioWrite(lcd.gpioHandle, dPIN_CS, 0);
    Lcd_Write8(command);
    lgGpioWrite(lcd.gpioHandle, dPIN_CS, 1);
}

/**************************************
/** @brief Envia 1 byte de dados para o display LCD.
* @param data: byte de dado.
* @retval Nenhum.
***************************************/
static void Lcd_DataByte(uint8_t data)
{
    lgGpioWrite(lcd.gpioHandle, dPIN_DC, 1);
    lgGpioWrite(lcd.gpioHandle, dPIN_CS, 0);
    Lcd_Write8(data);
    lgGpioWrite(lcd.gpioHandle, dPIN_CS, 1);
}

/**************************************
/** @brief Envia um comando seguido de um vetor de argumentos.
* @param command: codigo do comando.
* @param args: vetor com os argumentos.
* @param count: quantidade de argumentos.
* @retval Nenhum.
***************************************/
static void Lcd_Cmdv(uint8_t command, const uint8_t *args, int count)
{
    Lcd_Cmd(command);
    for(int i = 0; i < count; i++)
    {
        Lcd_DataByte(args[i]);
    }
}

/**************************************
/** @brief Tenta inicializar o acesso direto aos registradores RIO do RP1.
* @param Nenhum.
* @retval true se o acesso direto foi ativado com sucesso, false caso contrario.
***************************************/
static bool Lcd_FastInit(void)
{
    int fileDescriptor = open(dGPIOMEM_DEV, O_RDWR | O_SYNC);
    if(fileDescriptor < 0)
    {
        return false;
    }

    void *mappedAddr = mmap(NULL, dGPIOMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    close(fileDescriptor);

    if(mappedAddr == MAP_FAILED)
    {
        return false;
    }

    volatile uint32_t *rioPtr = (volatile uint32_t *)((char *)mappedAddr + dRIO_OFFSET);

    lcd.allMask = (1u << dPIN_DC) | (1u << dPIN_RST) | (1u << dPIN_CS) | (1u << dPIN_WR);
    for(int i = 0; i < dDATA_PINS_COUNT; i++)
    {
        lcd.allMask |= (1u << pinsData[i]);
    }

    if((rioPtr[1] & lcd.allMask) != lcd.allMask)
    {
        munmap(mappedAddr, dGPIOMEM_SIZE);
        return false;
    }

    for(int b = 0; b < dLUT_SIZE; b++)
    {
        uint32_t val = 0;
        for(int i = 0; i < dDATA_PINS_COUNT; i++)
        {
            if(((b >> i) & 1) != 0)
            {
                val |= (1u << pinsData[i]);
            }
        }
        lcd.lut[b] = val;
    }

    lcd.mapBase = mappedAddr;
    lcd.rio = rioPtr;
    return true;
}

/**************************************
/** @brief Transmite a matriz de pixels da janela especificada para o LCD.
* @param frameBuffer: ponteiro para o buffer de imagem.
* @param x0: coordenada X inicial.
* @param y0: coordenada Y inicial.
* @param x1: coordenada X final.
* @param y1: coordenada Y final.
* @retval Nenhum.
***************************************/
static void Lcd_PushPixels(const uint16_t *frameBuffer, int x0, int y0, int x1, int y1)
{
    lgGpioWrite(lcd.gpioHandle, dPIN_DC, 1);
    lgGpioWrite(lcd.gpioHandle, dPIN_CS, 0);

    if(lcd.rio != NULL)
    {
        const uint32_t writeMask = 1u << dPIN_WR;
        uint32_t baseVal = (lcd.rio[0] & ~lcd.allMask) | (1u << dPIN_DC) | (1u << dPIN_RST);

        for(int y = y0; y <= y1; y++)
        {
            const uint16_t *rowBuffer = frameBuffer + (y * dLCD_W);
            for(int x = x0; x <= x1; x++)
            {
                uint16_t color = rowBuffer[x];
                uint32_t highByteVal = baseVal | lcd.lut[color >> 8];

                lcd.rio[0] = highByteVal;
                lcd.rio[0] = highByteVal | writeMask;

                uint32_t lowByteVal = baseVal | lcd.lut[color & 0xFF];

                lcd.rio[0] = lowByteVal;
                lcd.rio[0] = lowByteVal | writeMask;
            }
        }
    }
    else
    {
        for(int y = y0; y <= y1; y++)
        {
            const uint16_t *rowBuffer = frameBuffer + (y * dLCD_W);
            for(int x = x0; x <= x1; x++)
            {
                uint16_t color = rowBuffer[x];
                Lcd_Write8((uint8_t)(color >> 8));
                Lcd_Write8((uint8_t)(color & 0xFF));
            }
        }
    }

    lgGpioWrite(lcd.gpioHandle, dPIN_CS, 1);
}
/** @} DOXYGEN GROUP TAG END OF FILE */