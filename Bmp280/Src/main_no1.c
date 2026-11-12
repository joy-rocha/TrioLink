/**************************************
* @file Main.c
* @brief Programa do No 1: le o sensor BMP280, calcula a media movel, monta
         o JSON e publica em sensor/bmp/raw a cada 2 segundos. Publica o
         proprio status em status/rasp1 e imprime os status dos outros nos.
* @details
* \n <b>Observacoes:</b>
* - Conexao COM TLS e USUARIO/SENHA configurados.
* - Encerrar com Ctrl+C (desconexao limpa, o Last Will NAO e disparado).
* - O payload de status segue o schema: {"timestamp":<ms>,"Status":"ONLINE"}.
* - Compilar com: -lmosquitto (ou a lib usada na MqttWrapper) -lcjson -lm
**************************************/

#include "MqttWrapper.h"
#include "BspSensorBmp280.h"
#include "SensorBmp280.h"
#include "JsonBmp280.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define dTEST_BROKER_HOST            "localhost"
#define dTEST_BROKER_PORT            8883
#define dTEST_CLIENT_ID              "no1"
#define dNODE1_MQTT_USERNAME         "no1"
#define dNODE1_MQTT_PASSWORD         "triolink"   // CONFIRMAR a senha do no1
#define dNODE1_CA_FILE_PATH          "/etc/mosquitto/certs/ca.crt" 

/// Topicos onde vamos escutar (status dos outros nos, conforme a tabela de topicos)
#define dTEST_TOPIC_SUB_STATUS_RASP2 "status/rasp2"
#define dTEST_TOPIC_SUB_STATUS_RASP3 "status/rasp3"

/// Topico onde vamos publicar os dados do BMP280 (QoS 0, sem retain)
#define dTEST_TOPIC_PUB              "sensor/bmp/raw"
/// Topico de status deste no (QoS 1, retain)
#define dTEST_TOPIC_STATUS           "status/rasp1"
#define dTEST_PUBLISH_PERIOD_SECONDS 2    
#define dTEST_CONNECT_TIMEOUT_MS     5000 
#define dTEST_CONNECT_POLL_MS        100  
/// Tamanho do buffer do JSON de status
#define dTEST_STATUS_BUFFER_SIZE     64
#define dTEST_MS_PER_SECOND          1000
#define dTEST_NS_PER_MS              1000000

static struct
{
    volatile sig_atomic_t stopRequested;
} mainTest;

static void Main_SignalHandler(int signalNumber);
static void Main_OnMessageReceived(const char *topic, const char *payload, u32 payloadLen);
static bool Main_WaitForConnection(void);
static bool Main_PublishSensorData(u32 counter);
static bool Main_BuildStatusPayload(char *outBuffer, u16 outBufferSize, const char *status);
static bool Main_PublishStatus(const char *status);

int main(void)
{
    int returnCode = 0;
    mqttWrapperConfig_t config;
    char willPayload[dTEST_STATUS_BUFFER_SIZE];
    u32 counter = 0;

    memset(&config, 0, sizeof(config));
    mainTest.stopRequested = 0;

    signal(SIGINT, Main_SignalHandler);

    strcpy(config.brokerHost, dTEST_BROKER_HOST);
    config.brokerPort = dTEST_BROKER_PORT;
    strcpy(config.clientId, dTEST_CLIENT_ID);
    
    strcpy(config.username, dNODE1_MQTT_USERNAME);
    strcpy(config.password, dNODE1_MQTT_PASSWORD);
    
    config.useTls = true;
    strncpy(config.caFilePath, dNODE1_CA_FILE_PATH, sizeof(config.caFilePath) - 1);

    // Last Will em JSON, conforme o schema (timestamp = instante da conexao)
    Main_BuildStatusPayload(willPayload, (u16)sizeof(willPayload), "OFFLINE");
    strcpy(config.willTopic, dTEST_TOPIC_STATUS);
    strcpy(config.willPayload, willPayload);
    config.willQos = eMQTT_WRAPPER_QOS_1;
    config.willRetain = true;
    config.messageCallback = Main_OnMessageReceived;

    printf("[main] Inicializando sensor BMP280 ...\n");

    if(BspSensorBmp280_Init() != eBSP_SENSOR_BMP280_RETURN_OK)
    {
        fprintf(stderr, "[main] ERRO: BspSensorBmp280_Init falhou. Sensor habilitado no device tree?\n");
        returnCode = 1;
    }
    else
    {
        printf("[main] Conectando em %s:%d ...\n", dTEST_BROKER_HOST, dTEST_BROKER_PORT);

        if(Mqtt_Wrapper_Init(&config) != eMQTT_WRAPPER_RETURN_OK)
        {
            fprintf(stderr, "[main] ERRO: Mqtt_Wrapper_Init falhou. O broker esta rodando?\n");
            returnCode = 1;
        }
        else if(Main_WaitForConnection() == false)
        {
            fprintf(stderr, "[main] ERRO: broker nao confirmou a conexao a tempo.\n");
            Mqtt_Wrapper_Deinit();
            returnCode = 1;
        }
        else
        {
            printf("[main] Conectado. Assinando %s e %s\n", dTEST_TOPIC_SUB_STATUS_RASP2, dTEST_TOPIC_SUB_STATUS_RASP3);

            if(Mqtt_Wrapper_Subscribe(dTEST_TOPIC_SUB_STATUS_RASP2, eMQTT_WRAPPER_QOS_1) != eMQTT_WRAPPER_RETURN_OK)
            {
                fprintf(stderr, "[main] AVISO: falha ao assinar %s\n", dTEST_TOPIC_SUB_STATUS_RASP2);
            }

            if(Mqtt_Wrapper_Subscribe(dTEST_TOPIC_SUB_STATUS_RASP3, eMQTT_WRAPPER_QOS_1) != eMQTT_WRAPPER_RETURN_OK)
            {
                fprintf(stderr, "[main] AVISO: falha ao assinar %s\n", dTEST_TOPIC_SUB_STATUS_RASP3);
            }

            Main_PublishStatus("ONLINE");

            while(mainTest.stopRequested == 0)
            {
                counter++;
                Main_PublishSensorData(counter);
                sleep(dTEST_PUBLISH_PERIOD_SECONDS);
            }

            printf("\n[main] Encerrando...\n");
            Main_PublishStatus("OFFLINE");
            Mqtt_Wrapper_Deinit();
        }
    }
    return returnCode;
}

static void Main_SignalHandler(int signalNumber)
{
    (void)signalNumber;
    mainTest.stopRequested = 1;
}

static void Main_OnMessageReceived(const char *topic, const char *payload, u32 payloadLen)
{
    printf("[callback] Recebido em '%s': %.*s\n", topic, (int)payloadLen, payload);
}

static bool Main_WaitForConnection(void)
{
    bool isConnected = false;
    u32 waitedMs = 0;

    while((isConnected == false) && (waitedMs < dTEST_CONNECT_TIMEOUT_MS))
    {
        Mqtt_Wrapper_IsConnected(&isConnected);

        if(isConnected == false)
        {
            usleep(dTEST_CONNECT_POLL_MS * 1000);
            waitedMs += dTEST_CONNECT_POLL_MS;
        }
    }
    return isConnected;
}

/**************************************
* @brief Monta o JSON de status: {"timestamp":<ms>,"Status":"<status>"}.
* @param outBuffer: buffer que recebe o texto (terminado em nulo).
* @param outBufferSize: tamanho do buffer, em bytes.
* @param status: "ONLINE" ou "OFFLINE".
* @retval true se o JSON foi montado; false caso contrario.
***************************************/
static bool Main_BuildStatusPayload(char *outBuffer, u16 outBufferSize, const char *status)
{
    bool isValid = false;
    struct timespec now;
    s64 timestampMs = 0;
    int written = 0;

    if(clock_gettime(CLOCK_REALTIME, &now) == 0)
    {
        timestampMs = ((s64)now.tv_sec * dTEST_MS_PER_SECOND) + ((s64)now.tv_nsec / dTEST_NS_PER_MS);
        written = snprintf(outBuffer, outBufferSize, "{\"timestamp\":%lld,\"Status\":\"%s\"}", (long long)timestampMs, status);

        if((written > 0) && (written < outBufferSize))
        {
            isValid = true;
        }
    }

    return isValid;
}

/**************************************
* @brief Publica o status deste no em status/rasp1 (QoS 1, retain).
* @param status: "ONLINE" ou "OFFLINE".
* @retval true se publicou com sucesso; false caso contrario.
***************************************/
static bool Main_PublishStatus(const char *status)
{
    bool isPublished = false;
    char payload[dTEST_STATUS_BUFFER_SIZE];

    if(Main_BuildStatusPayload(payload, (u16)sizeof(payload), status) == true)
    {
        if(Mqtt_Wrapper_Publish(dTEST_TOPIC_STATUS, payload, (u32)strlen(payload), eMQTT_WRAPPER_QOS_1, true) == eMQTT_WRAPPER_RETURN_OK)
        {
            isPublished = true;
        }
    }

    return isPublished;
}

/**************************************
* @brief Le o sensor, atualiza a media movel, monta o JSON e publica.
* @param counter: numero da iteracao (apenas para o log).
* @retval true se publicou com sucesso; false caso contrario.
***************************************/
static bool Main_PublishSensorData(u32 counter)
{
    bool isPublished = false;
    bool isConnected = false;
    char json[dJSON_BMP280_BUFFER_SIZE];
    u16 jsonLen = 0;
    sensorBmp280Data_t data;
    jsonBmp280Payload_t jsonPayload;

    Mqtt_Wrapper_IsConnected(&isConnected);

    if(SensorBmp280_Update() != eSENSOR_BMP280_RETURN_OK)
    {
        printf("[main] FALHA ao ler o sensor #%u\n", counter);
    }
    else if(SensorBmp280_GetAverageData(&data) == false)
    {
        printf("[main] FALHA: sem dados medios #%u\n", counter);
    }
    else
    {
        jsonPayload.timestamp = data.timestamp;
        jsonPayload.temperature = data.temperature;
        jsonPayload.pressure = data.pressure;
        jsonPayload.altitude = data.altitude;

        if(JsonBmp280_Build(&jsonPayload, json, (u16)sizeof(json), &jsonLen) != eJSON_BMP280_RETURN_OK)
        {
            printf("[main] FALHA ao montar o JSON #%u\n", counter);
        }
        else if(Mqtt_Wrapper_Publish(dTEST_TOPIC_PUB, json, (u32)jsonLen, eMQTT_WRAPPER_QOS_0, false) == eMQTT_WRAPPER_RETURN_OK)
        {
            printf("[main] Publicado em %s #%u (conectado=%s): %s\n", dTEST_TOPIC_PUB, counter, (isConnected == true) ? "sim" : "nao", json);
            isPublished = true;
        }
        else
        {
            printf("[main] FALHA ao publicar #%u (conectado=%s)\n", counter, (isConnected == true) ? "sim" : "nao");
        }
    }

    return isPublished;
}
