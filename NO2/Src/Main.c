/**************************************
* @file Main.c
* @brief Programa principal do No 2 (MPU6500): le o sensor, publica
         sensor/mpu/raw, assina sensor/bmp/raw do No 1 e publica
         fusion/bmp_mpu e status/rasp2, com TLS e usuario/senha.
* @author Caio Oliveira
* @details
* \n <b>Ferramentas:</b> Raspberry Pi 5, Linux.
* \n <b>Dependencias:</b>
* - MqttWrapper, Mpu6500Iio, BmpTracker, MpuPayloads.
* - Senha lida da variavel de ambiente MQTT_PASSWORD (nunca fica no
*   codigo-fonte nem na lista de processos).
* \n <b>Observacoes:</b>
* - Limitacao conhecida: se o broker estiver fora do ar no exato momento
*   em que este programa inicia, Mqtt_Wrapper_Init() falha e o programa
*   encerra (ver discussao sobre mosquitto_connect_async na MqttWrapper).
*   Uma vez conectado, quedas de rede DURANTE a execucao sao toleradas
*   normalmente (reconexao automatica).
* - Limitacao conhecida da MqttWrapper: assinaturas nao sao refeitas
*   sozinhas apos uma reconexao (clean_session=true). Como contorno,
*   ESTE programa detecta a transicao desconectado->conectado no proprio
*   loop principal e reassina sensor/bmp/raw e republica o status ONLINE
*   nesse momento (ver Main_CheckConnectionTransition()).
* Changelog
* @version <b>1.0.0 - 17/10/2026</b> \n Caio Oliveira \n Primeira versao.
**************************************/

/**************************************
* INCLUDES
**************************************/
#include "MqttWrapper.h"
#include "Mpu6500Iio.h"
#include "BmpTracker.h"
#include "MpuPayloads.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**************************************
* DEFINES LOCAIS
**************************************/
#define dNODE2_BROKER_PORT              8883                 // [porta TLS]
#define dNODE2_CLIENT_ID                "no2_mpu6500"        // [string]
#define dNODE2_MQTT_USERNAME            "no2"                // [string, ver acl.conf]
#define dNODE2_CA_FILE_PATH             "/etc/mosquitto/certs/ca.crt"    // [string]
#define dNODE2_CALIB_FILE_PATH          "./mpu6500_calib.txt"            // [string]

#define dNODE2_TOPIC_BMP_RAW            "sensor/bmp/raw"        // [string]
#define dNODE2_TOPIC_MPU_RAW            "sensor/mpu/raw"        // [string]
#define dNODE2_TOPIC_FUSION             "fusion/bmp_mpu"        // [string]
#define dNODE2_TOPIC_STATUS             "status/rasp2"          // [string]

#define dNODE2_CALIB_SAMPLES            200   // [amostras, ~1s a 5ms cada]
#define dNODE2_AVG_SAMPLES              10    // [amostras por publicacao]
#define dNODE2_AVG_INTERVAL_MS          30    // [ms entre amostras da media]
#define dNODE2_PUBLISH_PERIOD_S         1     // [s]

#define dNODE2_CONNECT_TIMEOUT_MS       5000  // [ms]
#define dNODE2_CONNECT_POLL_MS          100   // [ms]

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
**************************************/
/// Variaveis internas do programa principal
static struct
{
    /// Setada pelo handler de SIGINT (Ctrl+C) para encerrar o loop principal
    volatile sig_atomic_t stopRequested;
    /// Estado de conexao observado no ciclo ANTERIOR do loop, usado para
    /// detectar a transicao desconectado->conectado (reconexao)
    bool wasConnected;
} mainNode2;

/**************************************
* PROTOTIPOS LOCAIS
**************************************/
static void Main_SignalHandler(int signalNumber);
static void Main_OnMqttMessage(const char *topic, const char *payload, u32 payloadLen);
static s64 Main_GetTimestampMs(void);
static bool Main_WaitForConnection(void);
static mqttWrapperReturn_t Main_PublishStatus(bool isOnline);
static mqttWrapperReturn_t Main_SetupSensor(void);
static void Main_CheckConnectionTransition(void);

/**************************************
* FUNCAO PRINCIPAL
**************************************/
int main(int argc, char **argv)
{
    int returnCode = 0;
    const char *brokerHost = (argc > 1) ? argv[1] : "localhost";
    const char *password = getenv("MQTT_PASSWORD");
    mqttWrapperConfig_t config;
    char willPayload[dMPU_PAYLOADS_STATUS_BUFFER_LEN];
    u32 willPayloadLen = 0;

    memset(&mainNode2, 0, sizeof(mainNode2));
    memset(&config, 0, sizeof(config));
    signal(SIGINT, Main_SignalHandler);
    signal(SIGTERM, Main_SignalHandler);

    if(password == dNULL)
    {
        fprintf(stderr, "[no2] ERRO: defina a senha em MQTT_PASSWORD=... antes de rodar.\n");
        returnCode = 1;
    }
    else if(Main_SetupSensor() != eMQTT_WRAPPER_RETURN_OK)
    {
        fprintf(stderr, "[no2] ERRO: falha ao preparar o sensor MPU6500.\n");
        returnCode = 1;
    }
    else if(BmpTracker_Init() != eBMP_TRACKER_RETURN_OK)
    {
        fprintf(stderr, "[no2] ERRO: falha ao iniciar o BmpTracker.\n");
        returnCode = 1;
    }
    else
    {
        // Monta o payload de "nascimento"/Last Will UMA VEZ, aqui no inicio.
        // O timestamp carregado nao reflete o instante real da queda (isso
        // so o broker saberia, e o protocolo MQTT nao permite isso) - e
        // apenas o instante em que o programa foi configurado.
        MpuPayloads_BuildStatus(Main_GetTimestampMs(), false, willPayload, sizeof(willPayload), &willPayloadLen);

        strncpy(config.brokerHost, brokerHost, sizeof(config.brokerHost) - 1);
        config.brokerPort = dNODE2_BROKER_PORT;
        strncpy(config.clientId, dNODE2_CLIENT_ID, sizeof(config.clientId) - 1);
        strncpy(config.username, dNODE2_MQTT_USERNAME, sizeof(config.username) - 1);
        strncpy(config.password, password, sizeof(config.password) - 1);
        config.useTls = true;
        strncpy(config.caFilePath, dNODE2_CA_FILE_PATH, sizeof(config.caFilePath) - 1);
        strncpy(config.willTopic, dNODE2_TOPIC_STATUS, sizeof(config.willTopic) - 1);
        strncpy(config.willPayload, willPayload, sizeof(config.willPayload) - 1);
        config.willQos = eMQTT_WRAPPER_QOS_1;
        config.willRetain = true;
        config.messageCallback = Main_OnMqttMessage;

        printf("[no2] Conectando em %s:%d (TLS)...\n", brokerHost, dNODE2_BROKER_PORT);

        if(Mqtt_Wrapper_Init(&config) != eMQTT_WRAPPER_RETURN_OK)
        {
            fprintf(stderr, "[no2] ERRO: Mqtt_Wrapper_Init falhou. Broker no ar? Caminho do ca.crt (%s) correto?\n", dNODE2_CA_FILE_PATH);
            returnCode = 1;
        }
        else if(Main_WaitForConnection() == false)
        {
            fprintf(stderr, "[no2] ERRO: broker nao confirmou a conexao a tempo.\n");
            Mqtt_Wrapper_Deinit();
            returnCode = 1;
        }
        else
        {
            printf("[no2] Conectado.\n");
            Mqtt_Wrapper_Subscribe(dNODE2_TOPIC_BMP_RAW, eMQTT_WRAPPER_QOS_0);
            Main_PublishStatus(true);
            mainNode2.wasConnected = true;

            while(mainNode2.stopRequested == 0)
            {
                mpu6500Sample_t mpuSample;
                bmpTrackerData_t bmpData;
                char json[dMPU_PAYLOADS_FUSION_BUFFER_LEN];
                u32 jsonLen = 0;

                Main_CheckConnectionTransition();

                if(Mpu6500Iio_ReadAveragedSample(dNODE2_AVG_SAMPLES, dNODE2_AVG_INTERVAL_MS, &mpuSample) == eMPU6500_IIO_RETURN_OK)
                {
                    if(MpuPayloads_BuildRaw(&mpuSample, json, sizeof(json), &jsonLen) == eMPU_PAYLOADS_RETURN_OK)
                    {
                        Mqtt_Wrapper_Publish(dNODE2_TOPIC_MPU_RAW, json, jsonLen, eMQTT_WRAPPER_QOS_0, false);
                        printf("Publiquei em sensor/mpu ve la pfv");
                    }

                    BmpTracker_GetLatest(Main_GetTimestampMs(), &bmpData);

                    if(MpuPayloads_BuildFusion(&mpuSample, &bmpData, json, sizeof(json), &jsonLen) == eMPU_PAYLOADS_RETURN_OK)
                    {
                        Mqtt_Wrapper_Publish(dNODE2_TOPIC_FUSION, json, jsonLen, eMQTT_WRAPPER_QOS_1, true);
                    }

                    printf("[no2] publicado (bmp.stale=%s)\n", (bmpData.stale == true) ? "true" : "false");
                }
                else
                {
                    fprintf(stderr, "[no2] AVISO: falha na leitura do sensor neste ciclo.\n");
                }

                sleep(dNODE2_PUBLISH_PERIOD_S);
            }

            printf("\n[no2] Encerrando...\n");
            Main_PublishStatus(false);
            Mqtt_Wrapper_Deinit();
        }
    }
    return returnCode;
}

/**************************************
* FUNCOES LOCAIS
**************************************/
/**************************************
/** @brief Handler de SIGINT/SIGTERM. Apenas sinaliza o encerramento.
* @param signalNumber: numero do sinal recebido (nao utilizado).
**************************************/
static void Main_SignalHandler(int signalNumber)
{
    (void)signalNumber;
    mainNode2.stopRequested = 1;
}

/**************************************
/** @brief Callback chamado pela MqttWrapper a cada mensagem recebida.
           Executa na THREAD DE REDE, nao na thread principal.
* @param topic: topico da mensagem.
* @param payload: conteudo (NAO garantido terminar em zero).
* @param payloadLen: tamanho do conteudo, em bytes.
**************************************/
static void Main_OnMqttMessage(const char *topic, const char *payload, u32 payloadLen)
{
    if(strcmp(topic, dNODE2_TOPIC_BMP_RAW) == 0)
    {
        if(BmpTracker_OnMessageReceived(payload, payloadLen, Main_GetTimestampMs()) != eBMP_TRACKER_RETURN_OK)
        {
            fprintf(stderr, "[no2] AVISO: sensor/bmp/raw recebido mas nao pode ser interpretado.\n");
        }
    }
}

/**************************************
/** @brief Retorna o instante atual em milissegundos desde a epoca Unix (UTC).
* @param Nenhum.
* @retval Timestamp atual, em milissegundos.
**************************************/
static s64 Main_GetTimestampMs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    return ((s64)ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

/**************************************
/** @brief Espera o broker confirmar a conexao (CONNACK), com timeout.
* @param Nenhum.
* @retval true se conectou dentro do tempo limite; false caso contrario.
**************************************/
static bool Main_WaitForConnection(void)
{
    bool isConnected = false;
    u32 waitedMs = 0;

    while((isConnected == false) && (waitedMs < dNODE2_CONNECT_TIMEOUT_MS))
    {
        Mqtt_Wrapper_IsConnected(&isConnected);

        if(isConnected == false)
        {
            usleep(dNODE2_CONNECT_POLL_MS * 1000);
            waitedMs += dNODE2_CONNECT_POLL_MS;
        }
    }
    return isConnected;
}

/**************************************
/** @brief Monta e publica o payload de status/rasp2 (retido, QoS 1).
* @param isOnline: true publica "ONLINE", false publica "OFFLINE".
* @retval Retorno de Mqtt_Wrapper_Publish().
**************************************/
static mqttWrapperReturn_t Main_PublishStatus(bool isOnline)
{
    char json[dMPU_PAYLOADS_STATUS_BUFFER_LEN];
    u32 jsonLen = 0;

    MpuPayloads_BuildStatus(Main_GetTimestampMs(), isOnline, json, sizeof(json), &jsonLen);

    return Mqtt_Wrapper_Publish(dNODE2_TOPIC_STATUS, json, jsonLen, eMQTT_WRAPPER_QOS_1, true);
}

/**************************************
/** @brief Inicializa o sensor e carrega a calibracao salva, calibrando e
           salvando se nenhum arquivo de calibracao for encontrado.
* @param Nenhum.
* @retval eMQTT_WRAPPER_RETURN_OK se o sensor ficou pronto para uso.
          eMQTT_WRAPPER_RETURN_CONNECTION_ERROR caso contrario (reaproveita
          o enum da MqttWrapper so para simplificar o retorno de main();
          nao representa um erro de rede aqui).
* @details Se precisar calibrar, o sensor DEVE estar parado e nivelado:
           isto bloqueia por ~1s (dNODE2_CALIB_SAMPLES amostras).
**************************************/
static mqttWrapperReturn_t Main_SetupSensor(void)
{
    mqttWrapperReturn_t result = eMQTT_WRAPPER_RETURN_CONNECTION_ERROR;

    if(Mpu6500Iio_Init() != eMPU6500_IIO_RETURN_OK)
    {
        fprintf(stderr, "[no2] ERRO: sensor MPU6500 nao encontrado no IIO.\n");
    }
    else if(Mpu6500Iio_LoadCalibration(dNODE2_CALIB_FILE_PATH) == eMPU6500_IIO_RETURN_OK)
    {
        printf("[no2] Calibracao carregada de %s\n", dNODE2_CALIB_FILE_PATH);
        result = eMQTT_WRAPPER_RETURN_OK;
    }
    else
    {
        printf("[no2] Sem calibracao salva. Calibrando agora (mantenha o sensor parado e nivelado)...\n");

        if(Mpu6500Iio_Calibrate(dNODE2_CALIB_SAMPLES) != eMPU6500_IIO_RETURN_OK)
        {
            fprintf(stderr, "[no2] ERRO: falha ao calibrar o sensor.\n");
        }
        else
        {
            Mpu6500Iio_SaveCalibration(dNODE2_CALIB_FILE_PATH);
            printf("[no2] Calibracao concluida e salva em %s\n", dNODE2_CALIB_FILE_PATH);
            result = eMQTT_WRAPPER_RETURN_OK;
        }
    }
    return result;
}

/**************************************
/** @brief Detecta transicoes no estado da conexao desde o ciclo anterior
           do loop principal e reage a elas: ao RECONECTAR, reassina
           sensor/bmp/raw e republica o status ONLINE, ja que a
           MqttWrapper (clean_session=true) nao faz isso sozinha.
* @param Nenhum.
* @retval Nenhum.
**************************************/
static void Main_CheckConnectionTransition(void)
{
    bool isConnectedNow = false;

    Mqtt_Wrapper_IsConnected(&isConnectedNow);

    if((isConnectedNow == true) && (mainNode2.wasConnected == false))
    {
        printf("[no2] *** reconectado: reassinando %s e republicando status ONLINE ***\n", dNODE2_TOPIC_BMP_RAW);
        Mqtt_Wrapper_Subscribe(dNODE2_TOPIC_BMP_RAW, eMQTT_WRAPPER_QOS_0);
        Main_PublishStatus(true);
    }
    else if((isConnectedNow == false) && (mainNode2.wasConnected == true))
    {
        printf("[no2] *** conexao perdida ***\n");
    }

    mainNode2.wasConnected = isConnectedNow;
}
