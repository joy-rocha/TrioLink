/**************************************
* @file Mpu6500Iio.h
* @addtogroup Mpu6500Iio
* @{
**************************************/
#ifndef _MPU6500_IIO_H_
#define _MPU6500_IIO_H_

/**************************************
* INCLUDES NECESSARIOS
**************************************/
#include "AssertTypes.h"

/**************************************
* CONFIGURACOES
**************************************/
/// Nome de dispositivo IIO procurado primeiro (chip real, compatible exato)
#define dMPU6500_IIO_PRIMARY_NAME          "mpu6500" // [string]

/// Nome de dispositivo IIO procurado caso o primario nao seja encontrado.
/// Usado quando o overlay generico "i2c-sensor,mpu6050" estiver ativo em vez
/// do overlay customizado com o compatible exato do MPU6500.
#define dMPU6500_IIO_FALLBACK_NAME         "mpu6050" // [string]

/// Habilita (dTRUE) ou desabilita (dFALSE) a tentativa de busca pelo nome
/// de fallback acima, caso o nome primario nao seja encontrado
#define dMPU6500_IIO_TRY_FALLBACK_NAME     dTRUE     // [dTRUE ou dFALSE]

/// Tamanho maximo do caminho usado para salvar/carregar a calibracao
#define dMPU6500_IIO_MAX_CALIB_PATH_LEN    256       // [bytes]

/**************************************
* DEFINES PUBLICOS
**************************************/
/* Nao ha defines publicos nesta biblioteca */

/**************************************
* TIPOS DE DADOS PUBLICOS
**************************************/
/// Possiveis retornos das funcoes desta biblioteca
typedef enum
{
    eMPU6500_IIO_RETURN_OK,
    eMPU6500_IIO_RETURN_INVALID_ARGUMENT,
    eMPU6500_IIO_RETURN_NOT_FOUND,
    eMPU6500_IIO_RETURN_NOT_INITIALIZED,
    eMPU6500_IIO_RETURN_READ_ERROR,
    eMPU6500_IIO_RETURN_FILE_ERROR,

    eMPU6500_IIO_RETURN_END_ENUM
} mpu6500IioReturn_t;

/// Amostra completa e ja convertida para unidades fisicas, no mesmo formato
/// esperado pelo schema MQTT do topico sensor/mpu/raw
typedef struct
{
    /// Instante da aquisicao da amostra, em milissegundos desde a epoca
    /// Unix (UTC). Em uma amostra obtida por Mpu6500Iio_ReadAveragedSample(),
    /// representa o instante em que a primeira leitura da janela foi feita.
    s64 timestampMs;
    /// Aceleracao no eixo X. Unidade: m/s^2
    float accelX;
    /// Aceleracao no eixo Y. Unidade: m/s^2
    float accelY;
    /// Aceleracao no eixo Z. Unidade: m/s^2
    float accelZ;
    /// Velocidade angular no eixo X. Unidade: rad/s
    float gyroX;
    /// Velocidade angular no eixo Y. Unidade: rad/s
    float gyroY;
    /// Velocidade angular no eixo Z. Unidade: rad/s
    float gyroZ;
} mpu6500Sample_t;

/**************************************
* PROTOTIPOS PUBLICOS
**************************************/
mpu6500IioReturn_t Mpu6500Iio_Init(void);
mpu6500IioReturn_t Mpu6500Iio_ReadSample(mpu6500Sample_t *sampleOut);
mpu6500IioReturn_t Mpu6500Iio_ReadAveragedSample(u16 numSamples, u16 sampleIntervalMs, mpu6500Sample_t *sampleOut);
mpu6500IioReturn_t Mpu6500Iio_Calibrate(u16 numSamples);
mpu6500IioReturn_t Mpu6500Iio_IsCalibrated(bool *isCalibrated);
mpu6500IioReturn_t Mpu6500Iio_SaveCalibration(const char *filePath);
mpu6500IioReturn_t Mpu6500Iio_LoadCalibration(const char *filePath);

#endif /* _MPU6500_IIO_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
