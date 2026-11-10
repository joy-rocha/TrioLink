/**************************************
* @file BspSensorBmp280.h
* @addtogroup BspSensorBmp280
* @{
***************************************/
#ifndef _BSP_SENSOR_BMP280_H_
#define _BSP_SENSOR_BMP280_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include "AssertTypes.h"

/**************************************
* CONFIGURACOES
***************************************/
/** @addtogroup bspSensorBmp280_appCfg Configuracoes da aplicacao.
* @brief Define as constantes de portabilidade da BSP do sensor BMP280.
* @{
***************************************/
/// Pasta do kernel Linux onde ficam os dispositivos IIO (sysfs)
#define dBSP_SENSOR_BMP280_IIO_PATH "/sys/bus/iio/devices"
/// Nome do chip como o driver do kernel o registra (conteudo do arquivo "name").
/// OBS: para um BME280 seria "bme280".
#define dBSP_SENSOR_BMP280_CHIP_NAME "bmp280"
/** @} */

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Retornos das funcoes da BSP
typedef enum bspSensorBmp280Return
{
    /// Operacao concluida com sucesso
    eBSP_SENSOR_BMP280_RETURN_OK,
    /// Nenhum dispositivo IIO com o nome do chip foi encontrado no kernel
    eBSP_SENSOR_BMP280_RETURN_DEVICE_NOT_FOUND,
    /// A biblioteca SensorBmp280 recusou a inicializacao
    eBSP_SENSOR_BMP280_RETURN_LIB_INIT_ERROR,

    eBSP_SENSOR_BMP280_RETURN_END_ENUM
} bspSensorBmp280Return_t;

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
bspSensorBmp280Return_t BspSensorBmp280_Init(void);

#endif /* _BSP_SENSOR_BMP280_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
