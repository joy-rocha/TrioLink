/**************************************
* @file SensorBmp280.c
* @addtogroup SensorBmp280
* @brief Tratamento dos dados do sensor BMP280: conversao de unidades,
*        calculo da altitude, ultima leitura e medias moveis.
* @author Seu Nome
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - AssertTypes;
* - SimpleMovingAvg.
* \n <b>Observacoes:</b>
* - Este modulo nao acessa hardware: a leitura e feita por uma funcao injetada
*   no Init (implementada pela BSP);
* - O timestamp de cada dado e o da aquisicao da leitura mais recente, e nao o
*   do momento em que o dado for enviado;
* - Update, GetLastData e GetAverageData nao sao reentrantes: se forem chamadas
*   de contextos diferentes, o acesso deve ser protegido pela aplicacao;
* - A altitude usa a atmosfera padrao e a pressao de referencia
*   dSENSOR_BMP280_SEA_LEVEL_PRESSURE_PA;
* - Requer a biblioteca matematica (-lm) por causa do powf.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "SensorBmp280.h"
#include "SimpleMovingAvg.h"
#include <math.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Menor pressao de referencia valida ao nivel do mar [Pa]
#define dSEA_LEVEL_PRESSURE_MIN_PA 80000
/// Maior pressao de referencia valida ao nivel do mar [Pa]
#define dSEA_LEVEL_PRESSURE_MAX_PA 110000
/// Milesimos de oC em 1 oC (o kernel entrega a temperatura em milesimos)
#define dMILLI_C_PER_C 1000.0f
/// Pa em 1 kPa (o kernel entrega a pressao em kPa)
#define dPA_PER_KPA 1000.0f
/// Fator da formula barometrica da atmosfera padrao [m]
#define dALTITUDE_FACTOR_M 44330.0f
/// Expoente da formula barometrica da atmosfera padrao (1 / 5,255)
#define dALTITUDE_EXPONENT 0.1903f

// Verificacao de integridade das configuracoes
#if (dSENSOR_BMP280_SEA_LEVEL_PRESSURE_PA < dSEA_LEVEL_PRESSURE_MIN_PA) || \
    (dSENSOR_BMP280_SEA_LEVEL_PRESSURE_PA > dSEA_LEVEL_PRESSURE_MAX_PA)
    #error "dSENSOR_BMP280_SEA_LEVEL_PRESSURE_PA fora da faixa permitida (80000 a 110000)."
#endif

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Variaveis internas do modulo
static struct
{
    /// Ultima leitura tratada (valores instantaneos, sem media)
    sensorBmp280Data_t last;
    /// Indica se ja existe ao menos uma leitura valida
    bool hasData;
    /// Medias moveis de cada grandeza
    struct
    {
        /// Media da temperatura
        simpleMovingAvg_t temperature;
        /// Media da pressao
        simpleMovingAvg_t pressure;
        /// Media da altitude
        simpleMovingAvg_t altitude;
    } average;
    /// Ponteiros de funcoes externas (hardware) injetados no Init
    struct
    {
        bool (*readSensor)(s64 *timestampMs, s32 *temperatureMilliC, float *pressureKpa);
    } functions;
} sensorBmp280;

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static float SensorBmp280_CalculateAltitude(float pressurePa);

/**************************************
* FUNCOES PUBLICAS
***************************************/
/**************************************
/** @brief Inicializa o modulo e recebe a funcao de leitura do hardware.
* @param readFunc: funcao que adquire uma leitura completa do sensor. Recebe
*        ponteiros para o timestamp (Unix ms, UTC), a temperatura (milesimos
*        de oC) e a pressao (kPa), e retorna true se a leitura foi obtida.
* @retval eSENSOR_BMP280_RETURN_OK em caso de sucesso;
*         eSENSOR_BMP280_RETURN_INVALID_ARGUMENT se readFunc for nulo.
* @details Zera a ultima leitura e as medias moveis.
***************************************/
sensorBmp280Return_t SensorBmp280_Init(bool (*readFunc)(s64 *timestampMs,
                                                        s32 *temperatureMilliC,
                                                        float *pressureKpa))
{
    if(readFunc == dNULL)
    {
        return eSENSOR_BMP280_RETURN_INVALID_ARGUMENT;
    }

    sensorBmp280.functions.readSensor = readFunc;

    sensorBmp280.last.timestamp = 0;
    sensorBmp280.last.temperature = 0.0f;
    sensorBmp280.last.pressure = 0.0f;
    sensorBmp280.last.altitude = 0.0f;
    sensorBmp280.hasData = false;

    SimpleMovingAvg_Init(&sensorBmp280.average.temperature);
    SimpleMovingAvg_Init(&sensorBmp280.average.pressure);
    SimpleMovingAvg_Init(&sensorBmp280.average.altitude);

    return eSENSOR_BMP280_RETURN_OK;
}

/**************************************
/** @brief Adquire uma nova leitura, trata os dados e atualiza as medias
*         moveis.
* @param Nenhum.
* @retval eSENSOR_BMP280_RETURN_OK em caso de sucesso;
*         eSENSOR_BMP280_RETURN_NOT_INITIALIZED se o Init nao foi chamado;
*         eSENSOR_BMP280_RETURN_READ_ERROR se a BSP falhou na leitura;
*         eSENSOR_BMP280_RETURN_INVALID_DATA se a pressao lida e invalida.
* @details Deve ser chamada periodicamente pela aplicacao (uma vez por
*          amostra). Em caso de erro, nenhum dado e alterado: a leitura
*          defeituosa nao entra nas medias.
***************************************/
sensorBmp280Return_t SensorBmp280_Update(void)
{
    sensorBmp280Return_t result = eSENSOR_BMP280_RETURN_OK;
    s64 timestampMs = 0;
    s32 temperatureMilliC = 0;
    float pressureKpa = 0.0f;
    float temperature = 0.0f;
    float pressure = 0.0f;
    float altitude = 0.0f;

    if(sensorBmp280.functions.readSensor == dNULL)
    {
        result = eSENSOR_BMP280_RETURN_NOT_INITIALIZED;
    }
    else if(sensorBmp280.functions.readSensor(&timestampMs,
                                              &temperatureMilliC,
                                              &pressureKpa) == false)
    {
        result = eSENSOR_BMP280_RETURN_READ_ERROR;
    }
    else if((pressureKpa > 0.0f) == false)
    {
        // Pressao <= 0 (ou NaN) e impossivel e contaminaria a soma da media movel
        result = eSENSOR_BMP280_RETURN_INVALID_DATA;
    }
    else
    {
        temperature = ((float)temperatureMilliC / dMILLI_C_PER_C);
        pressure = (pressureKpa * dPA_PER_KPA);
        altitude = SensorBmp280_CalculateAltitude(pressure);

        sensorBmp280.last.timestamp = timestampMs;
        sensorBmp280.last.temperature = temperature;
        sensorBmp280.last.pressure = pressure;
        sensorBmp280.last.altitude = altitude;
        sensorBmp280.hasData = true;

        SimpleMovingAvg_NewSample(&sensorBmp280.average.temperature, temperature);
        SimpleMovingAvg_NewSample(&sensorBmp280.average.pressure, pressure);
        SimpleMovingAvg_NewSample(&sensorBmp280.average.altitude, altitude);
    }

    return result;
}

/**************************************
/** @brief Entrega a ultima leitura tratada, sem media.
* @param outData: ponteiro que recebe os dados.
* @retval true se ha uma leitura valida; false se outData for nulo ou se
*         ainda nao houve nenhuma leitura (outData nao e alterado).
***************************************/
bool SensorBmp280_GetLastData(sensorBmp280Data_t *outData)
{
    bool isValid = false;

    if((outData != dNULL) && (sensorBmp280.hasData == true))
    {
        *outData = sensorBmp280.last;
        isValid = true;
    }

    return isValid;
}

/**************************************
/** @brief Entrega os dados tratados como medias moveis.
* @param outData: ponteiro que recebe os dados.
* @retval true se ha dados validos; false se outData for nulo ou se ainda nao
*         houve nenhuma leitura (outData nao e alterado).
* @details Temperatura, pressao e altitude sao as medias da janela. O
*          timestamp e o da aquisicao da leitura mais recente incluida nas
*          medias.
***************************************/
bool SensorBmp280_GetAverageData(sensorBmp280Data_t *outData)
{
    bool isValid = false;

    if((outData != dNULL) && (sensorBmp280.hasData == true))
    {
        outData->timestamp = sensorBmp280.last.timestamp;
        outData->temperature = SimpleMovingAvg_GetValue(&sensorBmp280.average.temperature);
        outData->pressure = SimpleMovingAvg_GetValue(&sensorBmp280.average.pressure);
        outData->altitude = SimpleMovingAvg_GetValue(&sensorBmp280.average.altitude);
        isValid = true;
    }

    return isValid;
}

/**************************************
* FUNCOES LOCAIS
***************************************/
/**************************************
/** @brief Calcula a altitude a partir da pressao, pela formula barometrica
*         da atmosfera padrao.
* @param pressurePa: pressao medida, em Pa (deve ser maior que zero).
* @retval Altitude em m, relativa a pressao de referencia ao nivel do mar.
***************************************/
static float SensorBmp280_CalculateAltitude(float pressurePa)
{
    float ratio = (pressurePa / (float)dSENSOR_BMP280_SEA_LEVEL_PRESSURE_PA);

    // h = 44330 * (1 - (P / P0) ^ 0,1903)
    return (dALTITUDE_FACTOR_M * (1.0f - powf(ratio, dALTITUDE_EXPONENT)));
}

/** @} DOXYGEN GROUP TAG END OF FILE */
