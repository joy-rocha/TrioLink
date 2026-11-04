/**************************************
* @file main.c
* @brief Programa principal do no BMP280: inicializa a Bsp (le o sensor de
*        verdade via kernel), trata os dados, monta o JSON e publica via MQTT
*        (MqttWrapper, biblioteca compartilhada do Desafio 2).
* @details Se o broker estiver indisponivel no boot, o programa continua
*          rodando: SensorBmp280_Update/PublisherBmp280_Publish seguem
*          normalmente, os JSONs ficam na fila local (PublisherBmp280) e a
*          aplicacao tenta reconectar ao MQTT periodicamente, a cada
*          dMAIN_MQTT_RECONNECT_RETRY_CYCLES ciclos.
***************************************/
#include "BspSensorBmp280.h"
#include "SensorBmp280.h"
#include "PublisherBmp280.h"
#include "MqttWrapper.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/// Quantidade de ciclos de leitura/publicacao antes do programa terminar
#define dMAIN_LOOP_CYCLES 30
/// Intervalo entre ciclos, em segundos
#define dMAIN_LOOP_INTERVAL_S 1
/// A cada quantos ciclos tentar (re)conectar ao broker, caso ainda nao esteja
/// conectado (ex.: broker estava fora do ar no boot do programa)
#define dMAIN_MQTT_RECONNECT_RETRY_CYCLES 10

/// Endereco (IP ou hostname) do broker MQTT.
/// TODO: trocar pelo endereco combinado com o grupo (onde o Mosquitto roda).
#define dMAIN_MQTT_BROKER_HOST "localhost"
/// Porta do broker (1883 = sem TLS; 8883 = com TLS)
#define dMAIN_MQTT_BROKER_PORT 1883
/// Identificador deste cliente perante o broker (deve ser unico na rede)
#define dMAIN_MQTT_CLIENT_ID "no1_bmp280"

/// Indica se a ultima tentativa de conexao ao broker teve sucesso
static bool mqttReady = false;

/**************************************
/** @brief Adapta a assinatura de publish esperada pelo PublisherBmp280 para
*         a assinatura do MqttWrapper.
* @param topic: topico MQTT.
* @param payload: texto JSON a publicar.
* @param payloadLength: tamanho do payload, em bytes.
* @retval true se a libmosquitto aceitou a mensagem para envio; false caso
*         contrario (ex.: MQTT ainda nao conectado). Quando retorna false, o
*         PublisherBmp280 mantem o payload na fila e tenta de novo depois.
***************************************/
static bool MqttPublishAdapter(const char *topic, const char *payload, u16 payloadLength)
{
    // sensor/bmp/raw e QoS 0 / sem retain, conforme a tabela de topicos do Schemas.md
    return (Mqtt_Wrapper_Publish(topic, payload, (u32)payloadLength,
                                 eMQTT_WRAPPER_QOS_0, false) == eMQTT_WRAPPER_RETURN_OK);
}

/**************************************
/** @brief Tenta inicializar/conectar o MqttWrapper ao broker.
* @param Nenhum.
* @retval Nenhum.
* @details So e chamada enquanto mqttReady for false: o MqttWrapper e
*          singleton, entao uma vez conectado a propria libmosquitto cuida da
*          reconexao automatica em caso de queda (ver MqttWrapper.c).
***************************************/
static void TryMqttConnect(void)
{
    mqttWrapperConfig_t config;
    mqttWrapperReturn_t result;

    memset(&config, 0, sizeof(config));
    snprintf(config.brokerHost, sizeof(config.brokerHost), "%s", dMAIN_MQTT_BROKER_HOST);
    config.brokerPort = dMAIN_MQTT_BROKER_PORT;
    snprintf(config.clientId, sizeof(config.clientId), "%s", dMAIN_MQTT_CLIENT_ID);
    // username/password/caFilePath ficam vazios ("") = sem autenticacao/TLS.
    // Ajuste aqui quando o grupo decidir o mecanismo de seguranca do MQTT.
    config.useTls = false;
    // Sem Last Will configurado por enquanto (ver nota no fim do arquivo).
    config.messageCallback = dNULL; // este no so publica, nao assina nada

    result = Mqtt_Wrapper_Init(&config);

    if(result == eMQTT_WRAPPER_RETURN_OK)
    {
        mqttReady = true;
        printf("MQTT conectado ao broker %s:%u\n", dMAIN_MQTT_BROKER_HOST, (unsigned)dMAIN_MQTT_BROKER_PORT);
    }
    else if(result != eMQTT_WRAPPER_RETURN_ALREADY_INITIALIZED)
    {
        printf("AVISO: MQTT nao conectou agora (codigo %d). Os dados ficam na fila local; nova tentativa em breve.\n",
               (int)result);
    }
}

int main(void)
{
    int cycle;

    printf("=== No BMP280 (TrioLink - Desafio 1) ===\n\n");

    if(BspSensorBmp280_Init() != eBSP_SENSOR_BMP280_RETURN_OK)
    {
        printf("ERRO: sensor BMP280 nao encontrado no kernel.\n");
        printf("Confira o overlay (config.txt) e o endereco I2C.\n");
        return 1;
    }
    printf("Sensor BMP280 encontrado.\n");

    TryMqttConnect();

    // PublisherBmp280 recebe o adapter independentemente do MQTT ja estar
    // conectado: enquanto mqttReady for false, MqttPublishAdapter retorna
    // false e o PublisherBmp280 mantem os payloads na fila (ver PublisherBmp280.c).
    if(PublisherBmp280_Init(MqttPublishAdapter) != ePUBLISHER_BMP280_RETURN_OK)
    {
        printf("ERRO: PublisherBmp280_Init falhou.\n");
        return 1;
    }
    printf("Publisher inicializado.\n\n");

    for(cycle = 1; cycle <= dMAIN_LOOP_CYCLES; cycle++)
    {
        sensorBmp280Return_t updateResult;

        printf("Ciclo %d:\n", cycle);

        // Sem MQTT ainda: tenta reconectar periodicamente, sem travar o ciclo
        // de leitura do sensor.
        if((mqttReady == false) && ((cycle % dMAIN_MQTT_RECONNECT_RETRY_CYCLES) == 0))
        {
            TryMqttConnect();
        }

        updateResult = SensorBmp280_Update();

        if(updateResult != eSENSOR_BMP280_RETURN_OK)
        {
            printf("  -> SensorBmp280_Update falhou (codigo %d)\n\n", (int)updateResult);
        }
        else
        {
            publisherBmp280Return_t publishResult = PublisherBmp280_Publish();

            if(publishResult != ePUBLISHER_BMP280_RETURN_OK)
            {
                printf("  -> PublisherBmp280_Publish falhou (codigo %d)\n\n", (int)publishResult);
            }
            else
            {
                printf("  -> OK (publicado ou enfileirado para sensor/bmp/raw)\n\n");
            }
        }

        sleep(dMAIN_LOOP_INTERVAL_S);
    }

    if(mqttReady == true)
    {
        Mqtt_Wrapper_Deinit();
    }

    return 0;
}

/*
 * NOTA sobre status/rasp1 (Schemas.md): o schema define que o No 1 tambem
 * publica seu proprio status ("ONLINE"/"OFFLINE") no topico status/rasp1,
 * com QoS 1 e retain. O MqttWrapper ja suporta isso nativamente via Last
 * Will (campos willTopic/willPayload/willQos/willRetain de
 * mqttWrapperConfig_t, configurados em TryMqttConnect antes do connect): o
 * broker publica o willPayload sozinho se este cliente cair sem avisar.
 * Isso nao foi implementado aqui porque nao foi pedido ainda -- se quiser,
 * eu monto esse pedaco (configura o Last Will com status "OFFLINE" e publica
 * "ONLINE" manualmente logo apos o Init ter sucesso).
 */
