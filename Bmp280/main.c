/**************************************
* @file main.c
* @brief Programa principal do no BMP280: inicializa a Bsp (le o sensor de
*        verdade via kernel), trata os dados, monta o JSON (JsonBmp280) e
*        publica via MQTT (MqttWrapper, biblioteca compartilhada do
*        Desafio 2). Usa apenas BspSensorBmp280, SensorBmp280, JsonBmp280 e
*        MqttWrapper -- sem nenhum modulo de fila/retry.
* @details Se o broker estiver indisponivel no boot, o programa continua
*          rodando: SensorBmp280_Update segue normalmente, mas o JSON so e
*          publicado nos ciclos em que o MQTT estiver conectado (sem fila
*          local, o JSON de um ciclo sem conexao e apenas descartado). A
*          aplicacao tenta reconectar ao MQTT periodicamente, a cada
*          dMAIN_MQTT_RECONNECT_RETRY_CYCLES ciclos.
***************************************/
#include "BspSensorBmp280.h"
#include "SensorBmp280.h"
#include "JsonBmp280.h"
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

/// Topico MQTT deste dado, conforme a tabela de topicos do Schemas.md
#define dMAIN_MQTT_TOPIC_SENSOR_BMP_RAW "sensor/bmp/raw"

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
    // Sem Last Will configurado por enquanto.
    config.messageCallback = dNULL; // este no so publica, nao assina nada

    result = Mqtt_Wrapper_Init(&config);

    if(result == eMQTT_WRAPPER_RETURN_OK)
    {
        mqttReady = true;
        printf("MQTT conectado ao broker %s:%u\n", dMAIN_MQTT_BROKER_HOST, (unsigned)dMAIN_MQTT_BROKER_PORT);
    }
    else if(result != eMQTT_WRAPPER_RETURN_ALREADY_INITIALIZED)
    {
        printf("AVISO: MQTT nao conectou agora (codigo %d). Nova tentativa em breve.\n",
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
    printf("\n");

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
            sensorBmp280Data_t average;

            if(SensorBmp280_GetAverageData(&average) == false)
            {
                printf("  -> Ainda sem media disponivel\n\n");
            }
            else
            {
                jsonBmp280Payload_t payload;
                char jsonBuffer[dJSON_BMP280_BUFFER_SIZE];
                u16 jsonLength = 0;

                // sensorBmp280Data_t e jsonBmp280Payload_t tem os mesmos campos,
                // mas sao structs de arquivos diferentes (proposital, ver
                // JsonBmp280.c): a copia e feita campo a campo.
                payload.timestamp = average.timestamp;
                payload.temperature = average.temperature;
                payload.pressure = average.pressure;
                payload.altitude = average.altitude;

                if(JsonBmp280_Build(&payload, jsonBuffer, (u16)sizeof(jsonBuffer), &jsonLength) != eJSON_BMP280_RETURN_OK)
                {
                    printf("  -> JsonBmp280_Build falhou\n\n");
                }
                else
                {
                    printf("  -> JSON: %s\n", jsonBuffer);

                    if(mqttReady == true)
                    {
                        if(Mqtt_Wrapper_Publish(dMAIN_MQTT_TOPIC_SENSOR_BMP_RAW, jsonBuffer, jsonLength,
                                                eMQTT_WRAPPER_QOS_0, false) == eMQTT_WRAPPER_RETURN_OK)
                        {
                            printf("  -> Publicado em %s\n\n", dMAIN_MQTT_TOPIC_SENSOR_BMP_RAW);
                        }
                        else
                        {
                            printf("  -> Falha ao publicar\n\n");
                        }
                    }
                    else
                    {
                        printf("  -> MQTT nao conectado: dado nao publicado\n\n");
                    }
                }
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
