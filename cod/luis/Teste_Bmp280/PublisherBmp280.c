/**************************************
* @file PublisherBmp280.c
* @addtogroup PublisherBmp280
* @brief Ponto de integracao do no BMP280: pega os dados tratados do
*        SensorBmp280, monta o JSON com o JsonBmp280 e publica via MQTT,
*        com fila de reenvio para quando a rede/broker estiver indisponivel.
* @author Seu Nome
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - AssertTypes;
* - SensorBmp280;
* - JsonBmp280.
* \n <b>Observacoes:</b>
* - Este modulo NAO conhece Mosquitto, TLS ou qualquer detalhe de MQTT: a
*   funcao de envio e injetada no Init. Isso mantem este arquivo estavel mesmo quando o
*   mecanismo de seguranca do grupo mudar;
* - A fila fica em RAM: um reset durante uma desconexao perde os payloads
*   pendentes;
* - Se Publish e Flush forem chamadas de contextos diferentes (interrupcao e
*   laco principal), o acesso a fila deve ser protegido pela aplicacao;
* - Publica no topico sensor/bmp/raw (Schemas.md), QoS e retain sao definidos
*   por quem implementa mqttPublishFunc, nao por este arquivo.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "PublisherBmp280.h"
#include "SensorBmp280.h"
#include "JsonBmp280.h"

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Menor capacidade valida da fila
#define dQUEUE_SIZE_MIN 1
/// Maior capacidade valida da fila (indices e contador da fila sao u8)
#define dQUEUE_SIZE_MAX 255

// Verificacao de integridade das configuracoes
#if (dPUBLISHER_BMP280_QUEUE_SIZE < dQUEUE_SIZE_MIN) || (dPUBLISHER_BMP280_QUEUE_SIZE > dQUEUE_SIZE_MAX)
    #error "dPUBLISHER_BMP280_QUEUE_SIZE fora da faixa permitida (1 a 255)."
#endif

/**************************************
* CONSTANTES
***************************************/
/// Topico de publicacao dos payloads (Schemas.md)
static const char payloadTopic[] = "sensor/bmp/raw";

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Variaveis internas do modulo
static struct
{
    /// Fila circular de payloads pendentes de envio
    struct
    {
        /// Payloads armazenados, ja no formato de entrada do JsonBmp280
        jsonBmp280Payload_t items[dPUBLISHER_BMP280_QUEUE_SIZE];
        /// Posicao do item mais antigo (proximo a enviar)
        u8 head;
        /// Proxima posicao de escrita
        u8 tail;
        /// Quantidade de itens na fila
        u8 count;
    } queue;
    /// Ponteiros de funcoes externas (MQTT) injetados no Init
    struct
    {
        bool (*mqttPublish)(const char *topic, const char *payload, u16 payloadLength);
    } functions;
} publisherBmp280;

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static void PublisherBmp280_QueuePush(const jsonBmp280Payload_t *payload);
static const jsonBmp280Payload_t *PublisherBmp280_QueuePeek(void);
static void PublisherBmp280_QueuePop(void);

/**************************************
* FUNCOES PUBLICAS
***************************************/
/**************************************
/** @brief Inicializa o modulo e recebe a funcao de envio MQTT.
* @param mqttPublishFunc: funcao que publica um payload no topico informado e
*        retorna true se o envio foi confirmado pelo broker, false se a
*        publicacao falhou (ex.: sem conexao) e deve ser tentada depois.
* @retval ePUBLISHER_BMP280_RETURN_OK em caso de sucesso;
*         ePUBLISHER_BMP280_RETURN_INVALID_ARGUMENT se mqttPublishFunc for
*         nulo.
* @details Zera a fila de payloads pendentes.
***************************************/
publisherBmp280Return_t PublisherBmp280_Init(bool (*mqttPublishFunc)(const char *topic, const char *payload, u16 payloadLength))
{
    if(mqttPublishFunc == dNULL)
    {
        return ePUBLISHER_BMP280_RETURN_INVALID_ARGUMENT;
    }

    publisherBmp280.functions.mqttPublish = mqttPublishFunc;

    publisherBmp280.queue.head = 0;
    publisherBmp280.queue.tail = 0;
    publisherBmp280.queue.count = 0;

    return ePUBLISHER_BMP280_RETURN_OK;
}

/**************************************
/** @brief Monta o payload com a media atual do SensorBmp280, enfileira e
*         tenta publicar a fila.
* @param Nenhum.
* @retval ePUBLISHER_BMP280_RETURN_OK em caso de sucesso;
*         ePUBLISHER_BMP280_RETURN_NOT_INITIALIZED se o Init nao foi chamado;
*         ePUBLISHER_BMP280_RETURN_NO_DATA se o SensorBmp280 ainda nao tem
*         nenhuma leitura;
*         ePUBLISHER_BMP280_RETURN_JSON_ERROR se o JsonBmp280 nao conseguiu
*         montar o texto.
* @details Deve ser chamada a cada fechamento do intervalo de publicacao. Se
*          o MQTT estiver indisponivel, o payload permanece na fila e sera
*          enviado, em ordem cronologica, por uma chamada posterior (desta
*          funcao ou de PublisherBmp280_Flush).
* @warning Com a fila cheia, o payload mais antigo e descartado.
***************************************/
publisherBmp280Return_t PublisherBmp280_Publish(void)
{
    publisherBmp280Return_t result = ePUBLISHER_BMP280_RETURN_OK;
    sensorBmp280Data_t sensorData;
    jsonBmp280Payload_t payload;

    if(publisherBmp280.functions.mqttPublish == dNULL)
    {
        result = ePUBLISHER_BMP280_RETURN_NOT_INITIALIZED;
    }
    else if(SensorBmp280_GetAverageData(&sensorData) == false)
    {
        result = ePUBLISHER_BMP280_RETURN_NO_DATA;
    }
    else
    {
        payload.timestamp = sensorData.timestamp;
        payload.temperature = sensorData.temperature;
        payload.pressure = sensorData.pressure;
        payload.altitude = sensorData.altitude;

        PublisherBmp280_QueuePush(&payload);
        PublisherBmp280_Flush();
    }

    return result;
}

/**************************************
/** @brief Tenta publicar todos os payloads pendentes, do mais antigo ao mais
*         recente.
* @param Nenhum.
* @retval Nenhum.
* @details O item so e removido da fila apos a publicacao ser confirmada. Ao
*          primeiro erro (de envio ou de montagem do JSON) a funcao para e
*          mantem o restante da fila. Pode ser chamada periodicamente para
*          retomar o envio apos uma reconexao com o broker.
***************************************/
void PublisherBmp280_Flush(void)
{
    char json[dJSON_BMP280_BUFFER_SIZE];
    u16 jsonLength = 0;
    bool keepSending = true;
    const jsonBmp280Payload_t *payload = dNULL;

    // Sem funcao de envio injetada (Init nao chamado): nada a fazer
    if(publisherBmp280.functions.mqttPublish != dNULL)
    {
        while(keepSending == true)
        {
            payload = PublisherBmp280_QueuePeek();

            if(payload == dNULL)
            {
                // Fila vazia
                keepSending = false;
            }
            else if(JsonBmp280_Build(payload, json, (u16)sizeof(json), &jsonLength) != eJSON_BMP280_RETURN_OK)
            {
                // Payload nao serializavel: descarta para nao travar a fila
                PublisherBmp280_QueuePop();
            }
            else if(publisherBmp280.functions.mqttPublish(payloadTopic, json, jsonLength) == false)
            {
                // Sem conexao: mantem o item na fila e tenta novamente depois
                keepSending = false;
            }
            else
            {
                // Envio confirmado: remove o item da fila
                PublisherBmp280_QueuePop();
            }
        }
    }
}

/**************************************
* FUNCOES LOCAIS
***************************************/
/**************************************
/** @brief Insere um payload no fim da fila.
* @param payload: ponteiro do payload a ser copiado para a fila.
* @retval Nenhum.
* @warning Com a fila cheia, descarta o payload mais antigo para manter os
*          mais recentes.
***************************************/
static void PublisherBmp280_QueuePush(const jsonBmp280Payload_t *payload)
{
    if(publisherBmp280.queue.count == dPUBLISHER_BMP280_QUEUE_SIZE)
    {
        PublisherBmp280_QueuePop();
    }

    publisherBmp280.queue.items[publisherBmp280.queue.tail] = *payload;
    publisherBmp280.queue.tail = (u8)((publisherBmp280.queue.tail + 1) % dPUBLISHER_BMP280_QUEUE_SIZE);
    publisherBmp280.queue.count++;
}

/**************************************
/** @brief Consulta o payload mais antigo da fila sem remove-lo.
* @param Nenhum.
* @retval Ponteiro do payload mais antigo; dNULL se a fila estiver vazia.
***************************************/
static const jsonBmp280Payload_t *PublisherBmp280_QueuePeek(void)
{
    const jsonBmp280Payload_t *payload = dNULL;

    if(publisherBmp280.queue.count > 0)
    {
        payload = &publisherBmp280.queue.items[publisherBmp280.queue.head];
    }

    return payload;
}

/**************************************
/** @brief Remove o payload mais antigo da fila.
* @param Nenhum.
* @retval Nenhum.
***************************************/
static void PublisherBmp280_QueuePop(void)
{
    if(publisherBmp280.queue.count > 0)
    {
        publisherBmp280.queue.head = (u8)((publisherBmp280.queue.head + 1) % dPUBLISHER_BMP280_QUEUE_SIZE);
        publisherBmp280.queue.count--;
    }
}

/** @} DOXYGEN GROUP TAG END OF FILE */
