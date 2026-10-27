/**************************************
* @file BmpTracker.c
* @addtogroup BmpTracker
* @brief Guarda o ultimo dado valido recebido de sensor/bmp/raw (No 1) e
         calcula se ele esta obsoleto (stale), conforme a regra definida
         no Schemas.md do projeto.
* @author Caio Oliveira
* @details
* \n <b>Ferramentas:</b> generico (Linux, Raspberry Pi 5).
* \n <b>Dependencias:</b>
* - AssertTypes.
* - cJSON (link com -lcjson; header em <cjson/cJSON.h>).
* - pthread (link com -lpthread), para o mutex interno.
* \n <b>Observacoes:</b>
* - Biblioteca singleton, com UM mutex protegendo o dado compartilhado.
*   BmpTracker_OnMessageReceived() e pensada para ser chamada de dentro do
*   callback de mensagem da MqttWrapper (ou seja, na THREAD DE REDE), e
*   BmpTracker_GetLatest() e pensada para ser chamada do loop principal da
*   aplicacao (OUTRA thread). Sem o mutex, isso seria exatamente o tipo de
*   condicao de corrida descrita para o campo isConnected da MqttWrapper.
* - O payload recebido via MQTT NAO e garantido terminar em zero (ver
*   callback de mensagem da MqttWrapper): por isso o payload e copiado
*   para um buffer local com terminador antes de ser passado ao
*   cJSON_Parse(), que exige uma string C valida.
* - A staleness e calculada com base no RELOGIO LOCAL do No 2 no momento em
*   que a mensagem chegou (nowMs passado pelo chamador), nao no timestamp
*   embutido na mensagem do No 1. Isso evita depender de os relogios dos
*   dois Raspberry Pi estarem sincronizados entre si.
* Changelog
* @version <b>1.0.0 - 17/10/2026</b> \n Caio Oliveira \n Primeira versao.
* @copyright
**************************************/

/**************************************
* INCLUDES
**************************************/
#include "BmpTracker.h"
#include <cjson/cJSON.h>
#include <pthread.h>
#include <string.h>

/**************************************
* DEFINES LOCAIS
**************************************/
#define dBMP_TRACKER_FIELD_TIMESTAMP    "timestamp"   // [string]
#define dBMP_TRACKER_FIELD_TEMPERATURE  "temperature" // [string]
#define dBMP_TRACKER_FIELD_PRESSURE     "pressure"    // [string]
#define dBMP_TRACKER_FIELD_ALTITUDE     "altitude"    // [string]

/// Tamanho do buffer local usado para copiar o payload recebido e garantir
/// terminador nulo antes do parse (payloads maiores que isto sao rejeitados)
#define dBMP_TRACKER_PARSE_BUFFER_LEN   512           // [bytes]

/**************************************
* CONSTANTES
**************************************/
/* Nao ha constantes internas nesta biblioteca */

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
**************************************/
/// Variaveis internas da biblioteca: ultimo dado conhecido, protegido por
/// mutex contra acesso simultaneo da thread de rede (escrita) e da thread principal (leitura)
static struct
{
    /// Ultimo dado valido recebido (campos numericos; "stale" aqui nao e usado,
    /// e sempre recalculado em GetLatest a partir de lastReceivedAtMs)
    bmpTrackerData_t lastData;
    /// Instante (relogio local do No 2) em que a ultima mensagem valida
    /// foi recebida. Zero ate a primeira mensagem chegar, o que faz
    /// GetLatest reportar stale=true naturalmente antes disso.
    s64 lastReceivedAtMs;
    /// Protege lastData e lastReceivedAtMs
    pthread_mutex_t lock;
    /// Indica se BmpTracker_Init() ja foi chamada
    bool isInitialized;
} bmpTracker;

/**************************************
* PROTOTIPOS LOCAIS
**************************************/
static bmpTrackerReturn_t Bmp_Tracker_Parse(const char *json, bmpTrackerData_t *dataOut);

/**************************************
* FUNCOES PUBLICAS
**************************************/
/**************************************
/** @brief Inicializa o rastreador, com nenhum dado conhecido ainda.
* @param Nenhum.
* @retval eBMP_TRACKER_RETURN_OK sempre (nao ha condicao de falha aqui).
**************************************/
bmpTrackerReturn_t BmpTracker_Init(void)
{
    memset(&bmpTracker, 0, sizeof(bmpTracker));
    pthread_mutex_init(&bmpTracker.lock, dNULL);
    bmpTracker.isInitialized = true;

    return eBMP_TRACKER_RETURN_OK;
}

/**************************************
/** @brief Processa uma mensagem recebida em sensor/bmp/raw, atualizando o
           ultimo dado conhecido se o parse for bem sucedido.
* @param payload: conteudo da mensagem MQTT (NAO garantido terminar em zero).
* @param payloadLen: tamanho de payload, em bytes.
* @param nowMs: instante atual (relogio local do No 2), usado depois por
                BmpTracker_GetLatest() para calcular a staleness.
* @retval eBMP_TRACKER_RETURN_OK se a mensagem foi parseada e armazenada.
          eBMP_TRACKER_RETURN_INVALID_ARGUMENT se payload for dNULL ou
          payloadLen for zero ou maior que dBMP_TRACKER_PARSE_BUFFER_LEN - 1.
          eBMP_TRACKER_RETURN_NOT_INITIALIZED se Init() nao foi chamada.
          eBMP_TRACKER_RETURN_PARSE_ERROR se o JSON for invalido ou faltar
          algum campo esperado. Neste caso o dado anterior e preservado.
* @details Pensada para ser chamada de dentro do callback de mensagem
           configurado na MqttWrapper, apos assinar sensor/bmp/raw.
**************************************/
bmpTrackerReturn_t BmpTracker_OnMessageReceived(const char *payload, u32 payloadLen, s64 nowMs)
{
    bmpTrackerReturn_t result = eBMP_TRACKER_RETURN_INVALID_ARGUMENT;

    if((payload == dNULL) || (payloadLen == 0) || (payloadLen >= dBMP_TRACKER_PARSE_BUFFER_LEN))
    {
        result = eBMP_TRACKER_RETURN_INVALID_ARGUMENT;
    }
    else if(bmpTracker.isInitialized == false)
    {
        result = eBMP_TRACKER_RETURN_NOT_INITIALIZED;
    }
    else
    {
        char localBuffer[dBMP_TRACKER_PARSE_BUFFER_LEN];
        bmpTrackerData_t parsed;

        memcpy(localBuffer, payload, payloadLen);
        localBuffer[payloadLen] = '\0';

        result = Bmp_Tracker_Parse(localBuffer, &parsed);

        if(result == eBMP_TRACKER_RETURN_OK)
        {
            pthread_mutex_lock(&bmpTracker.lock);
            bmpTracker.lastData = parsed;
            bmpTracker.lastReceivedAtMs = nowMs;
            pthread_mutex_unlock(&bmpTracker.lock);
        }
    }
    return result;
}

/**************************************
/** @brief Devolve o ultimo dado conhecido de sensor/bmp/raw, com a flag
           "stale" ja calculada para o instante informado.
* @param nowMs: instante atual (relogio local do No 2).
* @param dataOut: ponteiro para a estrutura onde o dado sera escrito.
* @retval eBMP_TRACKER_RETURN_OK se a leitura foi realizada. Isto inclui o
          caso de nenhuma mensagem ter chegado ainda: dataOut vem zerado e
          stale=true, sem ser um erro.
          eBMP_TRACKER_RETURN_INVALID_ARGUMENT se dataOut for dNULL.
          eBMP_TRACKER_RETURN_NOT_INITIALIZED se Init() nao foi chamada.
**************************************/
bmpTrackerReturn_t BmpTracker_GetLatest(s64 nowMs, bmpTrackerData_t *dataOut)
{
    bmpTrackerReturn_t result = eBMP_TRACKER_RETURN_INVALID_ARGUMENT;

    if(dataOut == dNULL)
    {
        result = eBMP_TRACKER_RETURN_INVALID_ARGUMENT;
    }
    else if(bmpTracker.isInitialized == false)
    {
        result = eBMP_TRACKER_RETURN_NOT_INITIALIZED;
    }
    else
    {
        pthread_mutex_lock(&bmpTracker.lock);

        *dataOut = bmpTracker.lastData;
        dataOut->stale = ((nowMs - bmpTracker.lastReceivedAtMs) > dBMP_TRACKER_STALE_THRESHOLD_MS);

        pthread_mutex_unlock(&bmpTracker.lock);

        result = eBMP_TRACKER_RETURN_OK;
    }
    return result;
}

/**************************************
* FUNCOES LOCAIS
**************************************/
/**************************************
/** @brief Faz o parse de uma string JSON de sensor/bmp/raw.
* @param json: string C (terminada em zero) com o JSON recebido.
* @param dataOut: ponteiro para a estrutura onde o resultado sera escrito
                  (campo "stale" nao e preenchido aqui: e responsabilidade
                  de BmpTracker_GetLatest).
* @retval eBMP_TRACKER_RETURN_OK se todos os campos esperados foram lidos.
          eBMP_TRACKER_RETURN_PARSE_ERROR se o JSON for invalido ou faltar
          algum campo, ou algum campo nao for numerico.
**************************************/
static bmpTrackerReturn_t Bmp_Tracker_Parse(const char *json, bmpTrackerData_t *dataOut)
{
    bmpTrackerReturn_t result = eBMP_TRACKER_RETURN_PARSE_ERROR;
    cJSON *root = cJSON_Parse(json);

    if(root != dNULL)
    {
        cJSON *timestampItem   = cJSON_GetObjectItemCaseSensitive(root, dBMP_TRACKER_FIELD_TIMESTAMP);
        cJSON *temperatureItem = cJSON_GetObjectItemCaseSensitive(root, dBMP_TRACKER_FIELD_TEMPERATURE);
        cJSON *pressureItem    = cJSON_GetObjectItemCaseSensitive(root, dBMP_TRACKER_FIELD_PRESSURE);
        cJSON *altitudeItem    = cJSON_GetObjectItemCaseSensitive(root, dBMP_TRACKER_FIELD_ALTITUDE);

        if((cJSON_IsNumber(timestampItem) == true) && (cJSON_IsNumber(temperatureItem) == true) &&
           (cJSON_IsNumber(pressureItem) == true) && (cJSON_IsNumber(altitudeItem) == true))
        {
            dataOut->timestampMs = (s64)timestampItem->valuedouble;
            dataOut->temperature = (float)temperatureItem->valuedouble;
            dataOut->pressure    = (float)pressureItem->valuedouble;
            dataOut->altitude    = (float)altitudeItem->valuedouble;
            dataOut->stale       = false; // sobrescrito por GetLatest

            result = eBMP_TRACKER_RETURN_OK;
        }

        cJSON_Delete(root);
    }
    return result;
}
/** @} DOXYGEN GROUP TAG END OF FILE */
