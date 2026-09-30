/**************************************
* @file PublisherBmp280.h
* @addtogroup PublisherBmp280
* @{
***************************************/
#ifndef _PUBLISHER_BMP280_H_
#define _PUBLISHER_BMP280_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include "AssertTypes.h"

/**************************************
* CONFIGURACOES
***************************************/
/** @addtogroup publisherBmp280_appCfg Configuracoes da aplicacao.
* @brief Define as constantes do publicador MQTT do BMP280.
* @{
***************************************/
/// Capacidade da fila de payloads pendentes de envio.
/// Tempo maximo offline sem perda = capacidade x intervalo de publicacao.
#define dPUBLISHER_BMP280_QUEUE_SIZE 64 // [payloads] 1 a 255
/** @} */

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/
/// Retornos das funcoes do modulo
typedef enum publisherBmp280Return
{
    /// Operacao concluida com sucesso
    ePUBLISHER_BMP280_RETURN_OK,
    /// Argumento invalido (ex.: ponteiro de funcao nulo no Init)
    ePUBLISHER_BMP280_RETURN_INVALID_ARGUMENT,
    /// Init ainda nao foi chamado
    ePUBLISHER_BMP280_RETURN_NOT_INITIALIZED,
    /// O SensorBmp280 ainda nao tem nenhuma leitura para publicar
    ePUBLISHER_BMP280_RETURN_NO_DATA,
    /// O JsonBmp280 falhou ao montar o JSON
    ePUBLISHER_BMP280_RETURN_JSON_ERROR,

    ePUBLISHER_BMP280_RETURN_END_ENUM
} publisherBmp280Return_t;

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
publisherBmp280Return_t PublisherBmp280_Init(bool (*mqttPublishFunc)(const char *topic, const char *payload, u16 payloadLength));
publisherBmp280Return_t PublisherBmp280_Publish(void);
void PublisherBmp280_Flush(void);

#endif /* _PUBLISHER_BMP280_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
