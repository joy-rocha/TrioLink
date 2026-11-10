/**************************************
* @file SensorBmp280.h
* @addtogroup SensorBmp280
* @{
***************************************/
#ifndef _SENSOR_BMP280_H_
#define _SENSOR_BMP280_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include "AssertTypes.h"

/**************************************
* CONFIGURACOES
***************************************/
/** @addtogroup sensorBmp280_appCfg Configuracoes da aplicacao.
* @brief Define as constantes do modulo de dados do sensor BMP280.
* @{
***************************************/
/// Pressao ao nivel do mar usada como referencia no calculo da altitude.
/// OBS: varia com o clima; ajuste com o valor local para maior exatidao.
#define dSENSOR_BMP280_SEA_LEVEL_PRESSURE_PA 101325 // [Pa] 80000 a 110000
/** @} */

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Retornos das funcoes do modulo
typedef enum sensorBmp280Return
{
    /// Operacao concluida com sucesso
    eSENSOR_BMP280_RETURN_OK,
    /// Argumento invalido (ex.: ponteiro de funcao nulo)
    eSENSOR_BMP280_RETURN_INVALID_ARGUMENT,
    /// Init ainda nao foi chamado
    eSENSOR_BMP280_RETURN_NOT_INITIALIZED,
    /// A BSP falhou ao ler o sensor
    eSENSOR_BMP280_RETURN_READ_ERROR,
    /// A BSP entregou um valor fisicamente impossivel (ex.: pressao <= 0)
    eSENSOR_BMP280_RETURN_INVALID_DATA,

    eSENSOR_BMP280_RETURN_END_ENUM
} sensorBmp280Return_t;

/// Dados tratados do sensor, ja nas unidades do schema sensor/bmp/raw
typedef struct
{
    /// Instante da aquisicao: Unix em milissegundos, UTC
    s64 timestamp;
    /// Temperatura [oC]
    float temperature;
    /// Pressao [Pa]
    float pressure;
    /// Altitude [m]
    float altitude;
} sensorBmp280Data_t;

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
sensorBmp280Return_t SensorBmp280_Init(bool (*readFunc)(s64 *timestampMs,
                                                        s32 *temperatureMilliC,
                                                        float *pressureKpa));
sensorBmp280Return_t SensorBmp280_Update(void);
bool SensorBmp280_GetLastData(sensorBmp280Data_t *outData);
bool SensorBmp280_GetAverageData(sensorBmp280Data_t *outData);

#endif /* _SENSOR_BMP280_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
