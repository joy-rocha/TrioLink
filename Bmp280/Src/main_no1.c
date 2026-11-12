/**************************************
* @file Main.c
* @brief Programa de teste da MqttWrapper: le o sensor BMP280, calcula a
         media movel, monta o JSON e publica a cada 2 segundos. Imprime
         tudo que receber.
* @details
* \n <b>Observacoes:</b>
* - Conexao COM TLS e USUARIO/SENHA configurados.
* - Encerrar com Ctrl+C (desconexao limpa, o Last Will NAO e disparado).
* - Compilar com: -lmosquitto (ou a lib usada na MqttWrapper) -lcjson -lm
**************************************/

#include "MqttWrapper.h"
#include "BspSensorBmp280.h"
#include "SensorBmp280.h"
#include "JsonBmp280.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define dTEST_BROKER_HOST            "localhost"
#define dTEST_BROKER_PORT            8883
#define dTEST_CLIENT_ID              "no3"
#define dNODE3_MQTT_USERNAME         "no3"      
#define dNODE3_CA_FILE_PATH          "/etc/mosquitto/certs/ca.crt" 

/// Topico onde vamos escutar (voce liberou no ACL)
#define dTEST_TOPIC_SUB              "fusion/bmp_mpu"
#define dTEST_TOPIC_SUB_Sensor_Status   "status/#" //rasp1 = bmp, rasp2 = mpu

/// Topico onde vamos publicar (no3 tem permissao de write neste topico)
#define dTEST_TOPIC_PUB              "fusion/final"
/// Topico do schema do BMP280 (confira se o ACL deste usuario permite write)
#define dTEST_TOPIC_PUB_BMP          "sensor/bmp/raw"
#define dTEST_TOPIC_STATUS           "status/rasp3"
#define dTEST_PUBLISH_PERIOD_SECONDS 2    
#define dTEST_CONNECT_TIMEOUT_MS     5000 
#define dTEST_CONNECT_POLL_MS        100  

static struct
{
    volatile sig_atomic_t stopRequested;
} mainTest;

static void Main_SignalHandler(int signalNumber);
static void Main_OnMessageReceived(const char *topic, const char *payload, u32 payloadLen);
static bool Main_WaitForConnection(void);
static bool Main_PublishSensorData(u32 counter);

int main(void)
{
    int returnCode = 0;
    mqttWrapperConfig_t config;
    u32 counter = 0;

    memset(&config, 0, sizeof(config));
    mainTest.stopRequested = 0;

    signal(SIGINT, Main_SignalHandler);

    strcpy(config.brokerHost, dTEST_BROKER_HOST);
    config.brokerPort = dTEST_BROKER_PORT;
    strcpy(config.clientId, dTEST_CLIENT_ID);
    
    strcpy(config.username, dNODE3_MQTT_USERNAME);
    strcpy(config.password, "triolink");
    
    config.useTls = true;
    strncpy(config.caFilePath, dNODE3_CA_FILE_PATH, sizeof(config.caFilePath) - 1);
    strcpy(config.willTopic, dTEST_TOPIC_STATUS);
    strcpy(config.willPayload, "OFFLINE");
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
            printf("[main] Conectado. Assinando %s\n", dTEST_TOPIC_SUB);

            if(Mqtt_Wrapper_Subscribe(dTEST_TOPIC_SUB, eMQTT_WRAPPER_QOS_0) != eMQTT_WRAPPER_RETURN_OK)
            {
                fprintf(stderr, "[main] AVISO: falha ao assinar %s\n", dTEST_TOPIC_SUB);
            }

            Mqtt_Wrapper_Publish(dTEST_TOPIC_STATUS, "ONLINE", 6, eMQTT_WRAPPER_QOS_1, true);

            while(mainTest.stopRequested == 0)
            {
                counter++;
                Main_PublishSensorData(counter);
                sleep(dTEST_PUBLISH_PERIOD_SECONDS);
            }

            printf("\n[main] Encerrando...\n");
            Mqtt_Wrapper_Publish(dTEST_TOPIC_STATUS, "OFFLINE", 7, eMQTT_WRAPPER_QOS_1, true);
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
        else if(Mqtt_Wrapper_Publish(dTEST_TOPIC_PUB_BMP, json, (u32)jsonLen, eMQTT_WRAPPER_QOS_0, false) == eMQTT_WRAPPER_RETURN_OK)
        {
            printf("[main] Publicado em %s #%u (conectado=%s): %s\n", dTEST_TOPIC_PUB_BMP, counter, (isConnected == true) ? "sim" : "nao", json);
            isPublished = true;
        }
        else
        {
            printf("[main] FALHA ao publicar #%u (conectado=%s)\n", counter, (isConnected == true) ? "sim" : "nao");
        }
    }

    return isPublished;
}
