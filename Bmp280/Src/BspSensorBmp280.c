/**************************************
* @file BspSensorBmp280.c
* @addtogroup BspSensorBmp280
* @brief BSP do sensor BMP280 no Raspberry Pi 5: le temperatura e pressao do
*        driver do kernel Linux (subsistema IIO, via sysfs).
* @author Luis Eduardo
* @details
* \n <b>Ferramentas:</b>
* - Raspberry Pi OS (Linux) com GCC.
* \n <b>Dependencias:</b>
* - AssertTypes;
* - SensorBmp280.
* \n <b>Observacoes:</b>
* - O sensor precisa estar habilitado no device tree (overlay i2c-sensor com
*   o parametro bmp280) para que o kernel crie a pasta
*   /sys/bus/iio/devices/iio:deviceN;
* - O numero N da pasta pode mudar entre boots, por isso o Init procura o
*   dispositivo pelo nome do chip, e nao por um caminho fixo;
* - O kernel entrega a temperatura em milesimos de oC e a pressao em kPa. Esta
*   BSP repassa os valores nessas unidades; a conversao e feita no
*   SensorBmp280;
* - O timestamp vem do relogio do sistema (Unix ms, UTC) no instante da
*   leitura. Depende de o Raspberry estar com a hora sincronizada (RTC ou NTP).
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "BspSensorBmp280.h"
#include "SensorBmp280.h"
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Tamanho maximo de um caminho do sysfs, incluindo o terminador nulo
#define dPATH_MAX_SIZE 128
/// Tamanho maximo do texto lido de um arquivo do sysfs, incluindo o terminador nulo
#define dFILE_TEXT_MAX_SIZE 32
/// Tamanho do terminador nulo de uma string
#define dTERMINATOR_SIZE 1
/// Prefixo do nome das pastas dos dispositivos IIO
#define dIIO_DEVICE_PREFIX "iio:device"
/// Base decimal usada na conversao de texto para numero
#define dDECIMAL_BASE 10
/// Milissegundos em um segundo
#define dMS_PER_SECOND 1000
/// Nanossegundos em um milissegundo
#define dNS_PER_MS 1000000

/**************************************
* CONSTANTES
***************************************/
/// Arquivo do sysfs com o nome do chip
static const char fileNameChip[] = "name";
/// Arquivo do sysfs com a temperatura [milesimos de oC]
static const char fileNameTemperature[] = "in_temp_input";
/// Arquivo do sysfs com a pressao [kPa]
static const char fileNamePressure[] = "in_pressure_input";

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Variaveis internas da BSP
static struct
{
    /// Caminho completo do arquivo de temperatura do sensor encontrado
    char temperaturePath[dPATH_MAX_SIZE];
    /// Caminho completo do arquivo de pressao do sensor encontrado
    char pressurePath[dPATH_MAX_SIZE];
    /// Indica se o sensor foi encontrado no Init
    bool isReady;
} bspSensorBmp280;

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static bool BspSensorBmp280_FindDevice(void);
static bool BspSensorBmp280_CheckDevice(const char *deviceDir);
static bool BspSensorBmp280_BuildPath(char *outPath, u16 outPathSize, const char *deviceDir, const char *fileName);
static bool BspSensorBmp280_ReadFile(const char *path, char *outText, u16 outTextSize);
static bool BspSensorBmp280_ReadTemperature(s32 *outTemperatureMilliC);
static bool BspSensorBmp280_ReadPressure(float *outPressureKpa);
static bool BspSensorBmp280_GetTimestampMs(s64 *outTimestampMs);
static bool BspSensorBmp280_Read(s64 *timestampMs, s32 *temperatureMilliC, float *pressureKpa);

/**************************************
* FUNCOES PUBLICAS
***************************************/
/**************************************
/** @brief Localiza o sensor no kernel e injeta a funcao de leitura no
*         SensorBmp280.
* @param Nenhum.
* @retval eBSP_SENSOR_BMP280_RETURN_OK em caso de sucesso;
*         eBSP_SENSOR_BMP280_RETURN_DEVICE_NOT_FOUND se o kernel nao expos o
*         sensor;
*         eBSP_SENSOR_BMP280_RETURN_LIB_INIT_ERROR se o SensorBmp280 recusou
*         a inicializacao.
* @details Deve ser chamada apenas pelo Bsp_Init(). Se o sensor nao for
*          encontrado, o SensorBmp280 nao eh inicializado.
***************************************/
bspSensorBmp280Return_t BspSensorBmp280_Init(void)
{
    bspSensorBmp280Return_t result = eBSP_SENSOR_BMP280_RETURN_OK;

    bspSensorBmp280.isReady = false;

    if(BspSensorBmp280_FindDevice() == false)
    {
        result = eBSP_SENSOR_BMP280_RETURN_DEVICE_NOT_FOUND;
    }
    else if(SensorBmp280_Init(BspSensorBmp280_Read) != eSENSOR_BMP280_RETURN_OK)
    {
        result = eBSP_SENSOR_BMP280_RETURN_LIB_INIT_ERROR;
    }
    else
    {
        bspSensorBmp280.isReady = true;
    }

    return result;
}

/**************************************
* FUNCOES LOCAIS
***************************************/
/**************************************
/** @brief Percorre os dispositivos IIO do kernel ate achar o BMP280.
* @param Nenhum.
* @retval true se o sensor foi encontrado; false caso contrario.
* @details Ao encontrar, os caminhos dos arquivos de temperatura e pressao
*          ficam guardados na struct local para as leituras seguintes.
***************************************/
static bool BspSensorBmp280_FindDevice(void)
{
    bool isFound = false;
    DIR *dir = opendir(dBSP_SENSOR_BMP280_IIO_PATH); /// abre /sys/bus/iio/devices como se fosse uma pasta
    struct dirent *entry = dNULL; /// pega a proxima entrada (uma subpasta ou arquivo)

    if(dir != dNULL)
    {
        entry = readdir(dir);

        while((entry != dNULL) && (isFound == false))
        {
            // Considera apenas as pastas iio:deviceN
            if(strncmp(entry->d_name, dIIO_DEVICE_PREFIX, strlen(dIIO_DEVICE_PREFIX)) == 0) /// entry->d_name o nome dessa entrada, ex.: "iio:device0" 
            {
                isFound = BspSensorBmp280_CheckDevice(entry->d_name);
            }

            entry = readdir(dir);
        }

        closedir(dir); /// fecha, como fclose fecha um arquivo
    }

    return isFound;
}

/**************************************
/** @brief Verifica se uma pasta iio:deviceN e o BMP280 e, se for, guarda os
*         caminhos dos arquivos de dados.
* @param deviceDir: nome da pasta do dispositivo (ex.: "iio:device0").
* @retval true se a pasta e do BMP280; false caso contrario.
***************************************/
static bool BspSensorBmp280_CheckDevice(const char *deviceDir)
{
    bool isBmp280 = false;
    char namePath[dPATH_MAX_SIZE];
    char chipName[dFILE_TEXT_MAX_SIZE];

    if((BspSensorBmp280_BuildPath(namePath, (u16)sizeof(namePath), deviceDir, fileNameChip) == true) &&
       (BspSensorBmp280_ReadFile(namePath, chipName, (u16)sizeof(chipName)) == true) &&
       (strcmp(chipName, dBSP_SENSOR_BMP280_CHIP_NAME) == 0) &&
       (BspSensorBmp280_BuildPath(bspSensorBmp280.temperaturePath, (u16)sizeof(bspSensorBmp280.temperaturePath), deviceDir, fileNameTemperature) == true) &&
       (BspSensorBmp280_BuildPath(bspSensorBmp280.pressurePath, (u16)sizeof(bspSensorBmp280.pressurePath), deviceDir, fileNamePressure) == true))
    {
        isBmp280 = true;
    }
    return isBmp280;
}

/**************************************
/** @brief Monta o caminho completo de um arquivo do sysfs.
* @param outPath: buffer que recebe o caminho.
* @param outPathSize: tamanho do buffer, em bytes.
* @param deviceDir: nome da pasta do dispositivo (ex.: "iio:device0").
* @param fileName: nome do arquivo dentro da pasta.
* @retval true se o caminho coube no buffer; false caso contrario.
***************************************/
static bool BspSensorBmp280_BuildPath(char *outPath, u16 outPathSize, const char *deviceDir, const char *fileName)
{
    bool isValid = false;
    int written = snprintf(outPath, outPathSize, "%s/%s/%s", dBSP_SENSOR_BMP280_IIO_PATH, deviceDir, fileName);
    // Sucesso apenas se nao houve erro e o texto coube no buffer
    if((written > 0) && (written < outPathSize))
    {
        isValid = true;
    }

    return isValid;
}

/**************************************
/** @brief Le o texto de um arquivo do sysfs, sem o '\n' final.
* @param path: caminho do arquivo.
* @param outText: buffer que recebe o texto (terminado em nulo).
* @param outTextSize: tamanho do buffer, em bytes.
* @retval true se leu ao menos um byte; false caso contrario.
***************************************/
static bool BspSensorBmp280_ReadFile(const char *path, char *outText, u16 outTextSize)
{
    bool isValid = false;
    ssize_t bytesRead = 0;
    int fd = open(path, O_RDONLY);

    if(fd >= 0)
    {
        // Reserva espaco para o terminador nulo
        bytesRead = read(fd, outText, (size_t)(outTextSize - dTERMINATOR_SIZE));
        close(fd);

        if(bytesRead > 0)
        {
            // Os arquivos do sysfs terminam com quebra de linha: remove
            if(outText[bytesRead - 1] == '\n')
            {
                bytesRead--;
            }

            outText[bytesRead] = '\0';
            isValid = true;
        }
    }

    return isValid;
}

/**************************************
/** @brief Le a temperatura do kernel.
* @param outTemperatureMilliC: recebe a temperatura em milesimos de oC.
* @retval true se leu e converteu o valor; false caso contrario.
***************************************/
static bool BspSensorBmp280_ReadTemperature(s32 *outTemperatureMilliC)
{
    bool isValid = false;
    char text[dFILE_TEXT_MAX_SIZE];
    char *endPtr = dNULL;
    long value = 0;

    if(BspSensorBmp280_ReadFile(bspSensorBmp280.temperaturePath,
                                text,
                                (u16)sizeof(text)) == true)
    {
        errno = 0;
        value = strtol(text, &endPtr, dDECIMAL_BASE);

        // Valido se converteu ao menos um digito e nao houve erro de faixa
        if((endPtr != text) && (errno == 0))
        {
            *outTemperatureMilliC = (s32)value;
            isValid = true;
        }
    }

    return isValid;
}

/**************************************
/** @brief Le a pressao do kernel.
* @param outPressureKpa: recebe a pressao em kPa.
* @retval true se leu e converteu o valor; false caso contrario.
***************************************/
static bool BspSensorBmp280_ReadPressure(float *outPressureKpa)
{
    bool isValid = false;
    char text[dFILE_TEXT_MAX_SIZE];
    char *endPtr = dNULL;
    float value = 0.0f;

    if(BspSensorBmp280_ReadFile(bspSensorBmp280.pressurePath,
                                text,
                                (u16)sizeof(text)) == true)
    {
        errno = 0;
        value = strtof(text, &endPtr);

        // Valido se converteu ao menos um digito e nao houve erro de faixa
        if((endPtr != text) && (errno == 0))
        {
            *outPressureKpa = value;
            isValid = true;
        }
    }

    return isValid;
}

/**************************************
/** @brief Le o relogio do sistema em Unix milissegundos, UTC.
* @param outTimestampMs: recebe o instante atual.
* @retval true se o relogio foi lido; false caso contrario.
***************************************/
static bool BspSensorBmp280_GetTimestampMs(s64 *outTimestampMs)
{
    bool isValid = false;
    struct timespec now;

    // CLOCK_REALTIME e o relogio de parede (Unix, UTC)
    if(clock_gettime(CLOCK_REALTIME, &now) == 0)
    {
        *outTimestampMs = ((s64)now.tv_sec * dMS_PER_SECOND) +
                          ((s64)now.tv_nsec / dNS_PER_MS);
        isValid = true;
    }

    return isValid;
}

/**************************************
/** @brief Faz uma aquisicao completa: temperatura, pressao e timestamp.
*         Esta e a funcao injetada no SensorBmp280.
* @param timestampMs: recebe o instante da aquisicao (Unix ms, UTC).
* @param temperatureMilliC: recebe a temperatura em milesimos de oC.
* @param pressureKpa: recebe a pressao em kPa.
* @retval true se todos os valores foram obtidos; false caso contrario (nesse
*         caso os parametros de saida nao devem ser usados).
* @details O timestamp e lido logo apos as duas leituras, para representar o
*          momento da aquisicao (e nao o do envio).
***************************************/
static bool BspSensorBmp280_Read(s64 *timestampMs,
                                 s32 *temperatureMilliC,
                                 float *pressureKpa)
{
    bool isValid = false;

    if((bspSensorBmp280.isReady == true) &&
       (timestampMs != dNULL) &&
       (temperatureMilliC != dNULL) &&
       (pressureKpa != dNULL))
    {
        if((BspSensorBmp280_ReadTemperature(temperatureMilliC) == true) &&
           (BspSensorBmp280_ReadPressure(pressureKpa) == true))
        {
            isValid = BspSensorBmp280_GetTimestampMs(timestampMs);
        }
    }

    return isValid;
}

/** @} DOXYGEN GROUP TAG END OF FILE */
