/**************************************
* @file Lcd.c
* @addtogroup Lcd
* @brief Driver paralelo de 8 bits para display ILI9341 na Raspberry Pi 5.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - lgpio
* - sys/mman
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
/// Chip GPIO padrao da Raspberry Pi
#define dLCD_GPIOCHIP      0
/// Pino BCM de selecao de Comando/Dado (DC/RS)
#define dLCD_PIN_DC        24
/// Pino BCM de Reset por hardware (RST)
#define dLCD_PIN_RST       25
/// Pino BCM de Chip Select (CS)
#define dLCD_PIN_CS        8
/// Pino BCM de Write Strobe (WR)
#define dLCD_PIN_WR        23

/// Mascara total do grupo de controle lgpio (D0..D7 + WR)
#define dLCD_GRP_MASK      0x1FFULL
/// Bit referente ao sinal WR no grupo lgpio
#define dLCD_GRP_WR        0x100ULL

/// Ficheiro de dispositivo de memoria de GPIO do RP1
#define dLCD_GPIOMEM_DEV   "/dev/gpiomem0"
/// Tamanho do bloco de memoria mapeado
#define dLCD_GPIOMEM_SIZE  0x30000
/// Deslocamento dos registradores RIO do banco 0
#define dLCD_RIO_OFFSET    0x10000

/// Quantidade de pinos no grupo lgpio
#define dLCD_GRP_PIN_COUNT  9
/// Quantidade de pinos no barramento de dados
#define dLCD_DATA_PIN_COUNT 8

/**************************************
* CONSTANTES
***************************************/
/// Mapeamento dos pinos BCM de dados (D0 ate D7)
static const int dLCD_PINS_DATA[dLCD_DATA_PIN_COUNT] = {12, 13, 16, 19, 20, 21, 26, 27};
/// Mapeamento dos pinos do grupo lgpio (D0..D7 + WR)
static const int dLCD_PINS_GROUP[dLCD_GRP_PIN_COUNT] = {12, 13, 16, 19, 20, 21, 26, 27, dLCD_PIN_WR};

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Estrutura de estado e controle interno do LCD
static struct
{
    int gpioHandle;
    int groupHandle;
    volatile uint32_t *rioRegister;
    void *mapBaseAddress;
    uint32_t lookUpTable[256];
    uint32_t allPinsMask;
    uint16_t previousFrame[dLCD_W * dLCD_H];
    bool havePreviousFrame;
} lcd = {
    .gpioHandle = -1,
    .groupHandle = -1,
    .rioRegister = NULL,
    .mapBaseAddress = NULL,
    .havePreviousFrame = false
};

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static inline void Lcd_Write8(uint8_t dataVal);
static void Lcd_Cmd(uint8_t command);
static void Lcd_DataByte(uint8_t dataVal);
static void Lcd_CmdArgs(uint8_t command, const uint8_t *argsPtr, int count);
static bool Lcd_FastInit(void);
static void Lcd_PushPixels(const uint16_t *frameBufferPtr, int startX, int startY, int endX, int endY);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
* @brief Inicializa o controlador do display ILI9341.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Lcd_Init(void)
{
    lcd.gpioHandle = lgGpiochipOpen(dLCD_GPIOCHIP);
    if(lcd.gpioHandle < 0)
    {
        fprintf(stderr, "lgGpiochipOpen falhou (%d)\n", lcd.gpioHandle);
        return -1;
    }

    lgGpioClaimOutput(lcd.gpioHandle, 0, dLCD_PIN_DC, 0);
    lgGpioClaimOutput(lcd.gpioHandle, 0, dLCD_PIN_RST, 1);
    lgGpioClaimOutput(lcd.gpioHandle, 0, dLCD_PIN_CS, 1);

    const int initialLevels[dLCD_GRP_PIN_COUNT] = {0, 0, 0, 0, 0, 0, 0, 0, 1};
    if(lgGroupClaimOutput(lcd.gpioHandle, 0, dLCD_GRP_PIN_COUNT, dLCD_PINS_GROUP, initialLevels) < 0)
    {
        fprintf(stderr, "lgGroupClaimOutput falhou\n");
        return -1;
    }
    lcd.groupHandle = dLCD_PINS_GROUP[0];

    if(Lcd_FastInit() == true)
    {
        fprintf(stderr, "LCD: modo rapido (registradores RP1)\n");
    }
    else
    {
        fprintf(stderr, "LCD: modo lgpio (sem acesso a %s)\n", dLCD_GPIOMEM_DEV);
    }

    /* Reset por Hardware */
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_RST, 1);
    usleep(10000);
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_RST, 0);
    usleep(20000);
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_RST, 1);
    usleep(120000);

    /* Inicializacao do ILI9341 */
    const uint8_t initCmd1[] = {0x03, 0x80, 0x02};
    Lcd_CmdArgs(0xEF, initCmd1, 3);
    const uint8_t initCmd2[] = {0x00, 0xC1, 0x30};
    Lcd_CmdArgs(0xCF, initCmd2, 3);
    const uint8_t initCmd3[] = {0x64, 0x03, 0x12, 0x81};
    Lcd_CmdArgs(0xED, initCmd3, 4);
    const uint8_t initCmd4[] = {0x85, 0x00, 0x78};
    Lcd_CmdArgs(0xE8, initCmd4, 3);
    const uint8_t initCmd5[] = {0x39, 0x2C, 0x00, 0x34, 0x02};
    Lcd_CmdArgs(0xCB, initCmd5, 5);
    const uint8_t initCmd6[] = {0x20};
    Lcd_CmdArgs(0xF7, initCmd6, 1);
    const uint8_t initCmd7[] = {0x00, 0x00};
    Lcd_CmdArgs(0xEA, initCmd7, 2);
    const uint8_t initCmd8[] = {0x23};
    Lcd_CmdArgs(0xC0, initCmd8, 1);
    const uint8_t initCmd9[] = {0x10};
    Lcd_CmdArgs(0xC1, initCmd9, 1);
    const uint8_t initCmd10[] = {0x3E, 0x28};
    Lcd_CmdArgs(0xC5, initCmd10, 2);
    const uint8_t initCmd11[] = {0x86};
    Lcd_CmdArgs(0xC7, initCmd11, 1);
    const uint8_t initCmd12[] = {0x28};
    Lcd_CmdArgs(0x36, initCmd12, 1); /* Orientacao Paisagem */
    const uint8_t initCmd13[] = {0x55};
    Lcd_CmdArgs(0x3A, initCmd13, 1); /* RGB565 */
    const uint8_t initCmd14[] = {0x00, 0x18};
    Lcd_CmdArgs(0xB1, initCmd14, 2);
    const uint8_t initCmd15[] = {0x08, 0x82, 0x27};
    Lcd_CmdArgs(0xB6, initCmd15, 3);
    const uint8_t initCmd16[] = {0x00};
    Lcd_CmdArgs(0xF2, initCmd16, 1);
    const uint8_t initCmd17[] = {0x01};
    Lcd_CmdArgs(0x26, initCmd17, 1);

    const uint8_t gamma1[] = {0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00};
    Lcd_CmdArgs(0xE0, gamma1, 15);
    const uint8_t gamma2[] = {0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F};
    Lcd_CmdArgs(0xE1, gamma2, 15);

    Lcd_Cmd(0x11);
    usleep(120000);
    Lcd_Cmd(0x29);
    usleep(20000);

    lcd.havePreviousFrame = false;
    return 0;
}

/**************************************
* @brief Envia apenas a regiao alterada do framebuffer para o display.
* @param frameBufferPtr: ponteiro para o buffer de imagem RGB565.
* @retval Nenhum.
***************************************/
void Lcd_Flush(const uint16_t *frameBufferPtr)
{
    int startX = 0;
    int startY = 0;
    int endX = dLCD_W - 1;
    int endY = dLCD_H - 1;

    if(lcd.havePreviousFrame == true)
    {
        startX = dLCD_W;
        startY = dLCD_H;
        endX = -1;
        endY = -1;

        for(int y = 0; y < dLCD_H; y++)
        {
            const uint16_t *currentBufferRow = frameBufferPtr + (y * dLCD_W);
            const uint16_t *previousBufferRow = lcd.previousFrame + (y * dLCD_W);

            if(memcmp(currentBufferRow, previousBufferRow, dLCD_W * sizeof(uint16_t)) == 0)
            {
                continue;
            }

            if(y < startY)
            {
                startY = y;
            }
            endY = y;

            for(int x = 0; x < dLCD_W; x++)
            {
                if(currentBufferRow[x] != previousBufferRow[x])
                {
                    if(x < startX)
                    {
                        startX = x;
                    }
                    break;
                }
            }

            for(int x = dLCD_W - 1; x >= 0; x--)
            {
                if(currentBufferRow[x] != previousBufferRow[x])
                {
                    if(x > endX)
                    {
                        endX = x;
                    }
                    break;
                }
            }
        }

        if(endX < 0)
        {
            return;
        }
    }

    memcpy(lcd.previousFrame, frameBufferPtr, sizeof(lcd.previousFrame));
    lcd.havePreviousFrame = true;

    uint8_t windowArgs[4];
    windowArgs[0] = (uint8_t)(startX >> 8);
    windowArgs[1] = (uint8_t)(startX & 0xFF);
    windowArgs[2] = (uint8_t)(endX >> 8);
    windowArgs[3] = (uint8_t)(endX & 0xFF);
    Lcd_CmdArgs(0x2A, windowArgs, 4);

    windowArgs[0] = (uint8_t)(startY >> 8);
    windowArgs[1] = (uint8_t)(startY & 0xFF);
    windowArgs[2] = (uint8_t)(endY >> 8);
    windowArgs[3] = (uint8_t)(endY & 0xFF);
    Lcd_CmdArgs(0x2B, windowArgs, 4);

    Lcd_Cmd(0x2C);
    Lcd_PushPixels(frameBufferPtr, startX, startY, endX, endY);
}

/**************************************
* @brief Libera os recursos do display e encerra a conexao GPIO/mmap.
* @param Nenhum.
* @retval Nenhum.
***************************************/
void Lcd_Close(void)
{
    if(lcd.mapBaseAddress != NULL)
    {
        munmap(lcd.mapBaseAddress, dLCD_GPIOMEM_SIZE);
        lcd.mapBaseAddress = NULL;
        lcd.rioRegister = NULL;
    }

    if(lcd.gpioHandle >= 0)
    {
        lgGpiochipClose(lcd.gpioHandle);
        lcd.gpioHandle = -1;
    }

    lcd.groupHandle = -1;
}

/**************************************
* @brief Inicializa a interface de touch XPT2046.
* @param Nenhum.
* @retval 0 em caso de sucesso.
***************************************/
int Touch_Init(void)
{
    return 0;
}

/**************************************
* @brief Le as coordenadas brutas do controlador de touch.
* @param rawX: ponteiro para armazenar o valor X bruto.
* @param rawY: ponteiro para armazenar o valor Y bruto.
* @retval false sempre (stub temporario).
***************************************/
bool Touch_ReadRaw(int *rawX, int *rawY)
{
    (void)rawX;
    (void)rawY;
    return false;
}

/**************************************
* @brief Le as coordenadas convertidas em pixels do touch.
* @param posX: ponteiro para armazenar a coordenada X em pixels.
* @param posY: ponteiro para armazenar a coordenada Y em pixels.
* @retval false sempre (stub temporario).
***************************************/
bool Touch_Read(int *posX, int *posY)
{
    (void)posX;
    (void)posY;
    return false;
}

/**************************************
* FUNCOES LOCAIS
***************************************/

/**************************************
* @brief Escreve 1 byte no barramento paralelo via lgpio.
* @param dataVal: byte a ser transmitido.
* @retval Nenhum.
***************************************/
static inline void Lcd_Write8(uint8_t dataVal)
{
    lgGroupWrite(lcd.gpioHandle, lcd.groupHandle, (uint64_t)dataVal, dLCD_GRP_MASK);
    lgGroupWrite(lcd.gpioHandle, lcd.groupHandle, (uint64_t)dataVal | dLCD_GRP_WR, dLCD_GRP_MASK);
}

/**************************************
* @brief Envia um comando de 8 bits para o ILI9341.
* @param command: codigo do comando.
* @retval Nenhum.
***************************************/
static void Lcd_Cmd(uint8_t command)
{
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_DC, 0);
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_CS, 0);
    Lcd_Write8(command);
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_CS, 1);
}

/**************************************
* @brief Envia um byte de dado para o ILI9341.
* @param dataVal: valor do dado.
* @retval Nenhum.
***************************************/
static void Lcd_DataByte(uint8_t dataVal)
{
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_DC, 1);
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_CS, 0);
    Lcd_Write8(dataVal);
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_CS, 1);
}

/**************************************
* @brief Envia um comando com lista de argumentos para o ILI9341.
* @param command: codigo do comando.
* @param argsPtr: ponteiro para o array de argumentos.
* @param count: quantidade de argumentos.
* @retval Nenhum.
***************************************/
static void Lcd_CmdArgs(uint8_t command, const uint8_t *argsPtr, int count)
{
    Lcd_Cmd(command);
    for(int i = 0; i < count; i++)
    {
        Lcd_DataByte(argsPtr[i]);
    }
}

/**************************************
* @brief Inicializa o acesso direto aos registradores do RP1 para escrita rapida.
* @param Nenhum.
* @retval true se a inicializacao rapida funcionou, false caso contrario.
***************************************/
static bool Lcd_FastInit(void)
{
    int fileDescriptor = open(dLCD_GPIOMEM_DEV, O_RDWR | O_SYNC);
    if(fileDescriptor < 0)
    {
        return false;
    }

    void *mappedAddr = mmap(NULL, dLCD_GPIOMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fileDescriptor, 0);
    close(fileDescriptor);

    if(mappedAddr == MAP_FAILED)
    {
        return false;
    }

    volatile uint32_t *rioPtr = (volatile uint32_t *)((char *)mappedAddr + dLCD_RIO_OFFSET);

    lcd.allPinsMask = (1u << dLCD_PIN_DC) | (1u << dLCD_PIN_RST) | (1u << dLCD_PIN_CS) | (1u << dLCD_PIN_WR);
    for(int i = 0; i < dLCD_DATA_PIN_COUNT; i++)
    {
        lcd.allPinsMask |= (1u << dLCD_PINS_DATA[i]);
    }

    if((rioPtr[1] & lcd.allPinsMask) != lcd.allPinsMask)
    {
        munmap(mappedAddr, dLCD_GPIOMEM_SIZE);
        return false;
    }

    for(int b = 0; b < 256; b++)
    {
        uint32_t maskVal = 0;
        for(int i = 0; i < dLCD_DATA_PIN_COUNT; i++)
        {
            if(((b >> i) & 1) != 0)
            {
                maskVal |= (1u << dLCD_PINS_DATA[i]);
            }
        }
        lcd.lookUpTable[b] = maskVal;
    }

    lcd.mapBaseAddress = mappedAddr;
    lcd.rioRegister = rioPtr;
    return true;
}

/**************************************
* @brief Transmite o bloco de pixels da janela ativa para a memoria do LCD.
* @param frameBufferPtr: ponteiro para o framebuffer.
* @param startX: coordenada X inicial.
* @param startY: coordenada Y inicial.
* @param endX: coordenada X final.
* @param endY: coordenada Y final.
* @retval Nenhum.
***************************************/
static void Lcd_PushPixels(const uint16_t *frameBufferPtr, int startX, int startY, int endX, int endY)
{
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_DC, 1);
    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_CS, 0);

    if(lcd.rioRegister != NULL)
    {
        const uint32_t wrMask = (1u << dLCD_PIN_WR);
        uint32_t baseMask = (lcd.rioRegister[0] & ~lcd.allPinsMask) | (1u << dLCD_PIN_DC) | (1u << dLCD_PIN_RST);

        for(int y = startY; y <= endY; y++)
        {
            const uint16_t *rowPtr = frameBufferPtr + (y * dLCD_W);
            for(int x = startX; x <= endX; x++)
            {
                uint16_t colorVal = rowPtr[x];
                uint32_t pinVal = baseMask | lcd.lookUpTable[colorVal >> 8];
                lcd.rioRegister[0] = pinVal;
                lcd.rioRegister[0] = pinVal | wrMask;

                pinVal = baseMask | lcd.lookUpTable[colorVal & 0xFF];
                lcd.rioRegister[0] = pinVal;
                lcd.rioRegister[0] = pinVal | wrMask;
            }
        }
    }
    else
    {
        for(int y = startY; y <= endY; y++)
        {
            const uint16_t *rowPtr = frameBufferPtr + (y * dLCD_W);
            for(int x = startX; x <= endX; x++)
            {
                uint16_t colorVal = rowPtr[x];
                Lcd_Write8((uint8_t)(colorVal >> 8));
                Lcd_Write8((uint8_t)(colorVal & 0xFF));
            }
        }
    }

    lgGpioWrite(lcd.gpioHandle, dLCD_PIN_CS, 1);
}
/** @} DOXYGEN GROUP TAG END OF FILE */