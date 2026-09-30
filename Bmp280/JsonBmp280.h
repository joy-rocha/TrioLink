/**************************************
* @file JsonBmp280.h
* @addtogroup JsonBmp280
* @{
***************************************/
#ifndef _JSON_BMP280_H_
#define _JSON_BMP280_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include "AssertTypes.h"

/**************************************
* DEFINES PUBLICOS
***************************************/
/// Tamanho de buffer recomendado para receber o JSON, em bytes (o JSON tem
/// no maximo cerca de 90 bytes; a folga atende a exigencia do cJSON).
#define dJSON_BMP280_BUFFER_SIZE 128

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Retornos das funcoes do modulo
typedef enum jsonBmp280Return
{
    /// JSON gerado com sucesso
    eJSON_BMP280_RETURN_OK,
    /// Argumento invalido (ponteiro nulo ou tamanho de buffer zero)
    eJSON_BMP280_RETURN_INVALID_ARGUMENT,
    /// O JSON nao coube no buffer informado
    eJSON_BMP280_RETURN_BUFFER_TOO_SMALL,
    /// Falha interna do cJSON (ex.: sem memoria)
    eJSON_BMP280_RETURN_ERROR,

    eJSON_BMP280_RETURN_END_ENUM
} jsonBmp280Return_t;

/// Dados de entrada do JSON, conforme o schema do topico sensor/bmp/raw.
/// OBS: definido aqui (e nao reaproveitado de outro modulo) de proposito,
/// para que este arquivo nao dependa de nenhuma outra lib alem do cJSON.
typedef struct
{
    /// Unix em milissegundos, UTC
    s64 timestamp;
    /// Temperatura [oC]
    float temperature;
    /// Pressao [Pa]
    float pressure;
    /// Altitude [m]
    float altitude;
} jsonBmp280Payload_t;

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
jsonBmp280Return_t JsonBmp280_Build(const jsonBmp280Payload_t *payload, char *outBuffer, u16 outBufferSize, u16 *outLength);

#endif /* _JSON_BMP280_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
