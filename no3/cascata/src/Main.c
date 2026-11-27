/*
 * @file    Main.c
 * @brief   Integra os modulos do no de display (No 3): GPIO, botoes, MQTT
 *          (MqttWrapper + Fusion) e interface (BspScreen).
 * @details
 *  - Este no e tambem o broker Mosquitto: host padrao "localhost" e porta
 *    TLS 8883. Uso: ./cascata [host] [porta]
 *  - A senha vem da variavel de ambiente MQTT_PASSWORD (ou o valor padrao).
 *  - UMA unica conexao MQTT (MqttWrapper, TLS + usuario/senha) faz tudo:
 *    recebe os dados (callback = Fusion_OnMessage), assina os topicos e
 *    publica status/rasp3 (ONLINE retido, Last Will OFFLINE).
 *  - Fluxo igual ao da main de teste: Init -> espera a conexao -> assina
 *    -> publica ONLINE. Como a sessao e limpa, a thread auxiliar refaz as
 *    assinaturas e o ONLINE a cada reconexao.
 */

#include "BspButton.h"
#include "BspGpio.h"
#include "BspScreen.h"
#include "Fusion.h"
#include "MqttWrapper.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/// Broker MQTT padrao (este no e o proprio broker) e porta TLS
#define dMAIN_BROKER_HOST           "localhost"
#define dMAIN_BROKER_PORT           8883

/// Posicao dos argumentos opcionais: ./cascata [host] [porta]
#define dMAIN_ARG_HOST              1
#define dMAIN_ARG_PORT              2

/// Credenciais e certificado da conexao segura
#define dMAIN_MQTT_CLIENT_ID        "no3_display"
#define dMAIN_MQTT_USERNAME         "no3"
#define dMAIN_MQTT_PASSWORD_ENV     "MQTT_PASSWORD"
#define dMAIN_MQTT_PASSWORD_DEFAULT "triolink"
#define dMAIN_CA_FILE_PATH          "/etc/mosquitto/certs/ca.crt"

/// Topico de status deste no e valores do campo Status
#define dMAIN_TOPIC_STATUS          "status/rasp3"
#define dMAIN_STATUS_ONLINE         "ONLINE"
#define dMAIN_STATUS_OFFLINE        "OFFLINE"

/// Tamanho do buffer do payload de status [bytes]
#define dMAIN_STATUS_BUFFER_LEN     64

/// Espera pela conexao inicial: tempo maximo e passo de verificacao [ms]
#define dMAIN_CONNECT_TIMEOUT_MS    5000
#define dMAIN_CONNECT_POLL_MS       100

/// Periodo de verificacao da conexao pela thread auxiliar [us]
#define dMAIN_WATCHDOG_PERIOD_US    200000

/// Tempo para o broker receber o OFFLINE antes de desconectar [us]
#define dMAIN_SHUTDOWN_FLUSH_US     300000

/// Conversoes de tempo
#define dMAIN_MS_PER_SECOND         1000
#define dMAIN_NS_PER_MS             1000000
#define dMAIN_US_PER_MS             1000

/// Variaveis internas do programa principal
static struct
{
    pthread_t watchdogThread;
    volatile int stopRequested;
    bool wasConnected;
} mainNode3;

static long long Main_GetTimestampMs(void);
static bool Main_PublishStatus(const char *status);
static bool Main_BrokerInit(const char *brokerHost, int brokerPort);
static bool Main_WaitForConnection(void);
static void Main_OnConnected(void);
static void Main_BrokerDeinit(void);
static void *Main_WatchdogThread(void *argument);

/*
 * @brief   Le o relogio de parede em milissegundos (Unix, UTC).
 * @retval  Tempo atual [ms]
 */
static long long Main_GetTimestampMs(void)
{
    struct timespec now;

    clock_gettime(CLOCK_REALTIME, &now);

    return ((long long)now.tv_sec * dMAIN_MS_PER_SECOND)
           + ((long long)now.tv_nsec / dMAIN_NS_PER_MS);
}

/*
 * @brief   Publica o status deste no (QoS 1, retido) em JSON.
 * @param   status - "ONLINE" ou "OFFLINE"
 * @retval  true  - Publicacao entregue a MqttWrapper
 * @retval  false - Falha ao montar ou publicar
 */
static bool Main_PublishStatus(const char *status)
{
    char payload[dMAIN_STATUS_BUFFER_LEN];
    int length = snprintf(payload, sizeof(payload),
                          "{\"timestamp\":%lld,\"Status\":\"%s\"}",
                          Main_GetTimestampMs(), status);

    if((length <= 0) || ((size_t)length >= sizeof(payload)))
    {
        return false;
    }

    return (Mqtt_Wrapper_Publish(dMAIN_TOPIC_STATUS, payload, (u32)length,
                                 eMQTT_WRAPPER_QOS_1, true)
            == eMQTT_WRAPPER_RETURN_OK);
}

/*
 * @brief   Configura e inicia a conexao com o broker (TLS, usuario/senha,
 *          Last Will de status/rasp3 e callback de mensagens do Fusion).
 * @param   brokerHost - Endereco do broker
 * @param   brokerPort - Porta TLS do broker
 * @retval  true  - Mqtt_Wrapper_Init OK
 * @retval  false - Falha ao iniciar
 */
static bool Main_BrokerInit(const char *brokerHost, int brokerPort)
{
    mqttWrapperConfig_t config;
    const char *password = getenv(dMAIN_MQTT_PASSWORD_ENV);

    if((password == NULL) || (password[0] == '\0'))
    {
        password = dMAIN_MQTT_PASSWORD_DEFAULT;
    }

    memset(&config, 0, sizeof(config));

    strncpy(config.brokerHost, brokerHost, sizeof(config.brokerHost) - 1);
    config.brokerPort = (u16)brokerPort;
    strncpy(config.clientId, dMAIN_MQTT_CLIENT_ID, sizeof(config.clientId) - 1);
    strncpy(config.username, dMAIN_MQTT_USERNAME, sizeof(config.username) - 1);
    strncpy(config.password, password, sizeof(config.password) - 1);
    config.useTls = true;
    strncpy(config.caFilePath, dMAIN_CA_FILE_PATH, sizeof(config.caFilePath) - 1);
    strncpy(config.willTopic, dMAIN_TOPIC_STATUS, sizeof(config.willTopic) - 1);
    (void)snprintf(config.willPayload, sizeof(config.willPayload),
                   "{\"timestamp\":%lld,\"Status\":\"%s\"}",
                   Main_GetTimestampMs(), dMAIN_STATUS_OFFLINE);
    config.willQos = eMQTT_WRAPPER_QOS_1;
    config.willRetain = true;
    config.messageCallback = Fusion_OnMessage;

    (void)printf("[no3] Conectando em %s:%d (TLS)...\n", brokerHost, brokerPort);

    return (Mqtt_Wrapper_Init(&config) == eMQTT_WRAPPER_RETURN_OK);
}

/*
 * @brief   Espera o broker confirmar a conexao (ate dMAIN_CONNECT_TIMEOUT_MS).
 * @retval  true  - Conectado
 * @retval  false - Tempo esgotado
 */
static bool Main_WaitForConnection(void)
{
    bool isConnected = false;
    u32 waitedMs = 0;

    while((isConnected == false) && (waitedMs < dMAIN_CONNECT_TIMEOUT_MS))
    {
        (void)Mqtt_Wrapper_IsConnected(&isConnected);

        if(isConnected == false)
        {
            usleep(dMAIN_CONNECT_POLL_MS * dMAIN_US_PER_MS);
            waitedMs += dMAIN_CONNECT_POLL_MS;
        }
    }

    return isConnected;
}

/*
 * @brief   Acoes a cada (re)conexao: assina os topicos e publica ONLINE.
 */
static void Main_OnConnected(void)
{
    if(Fusion_Subscribe() == false)
    {
        (void)fprintf(stderr, "[no3] AVISO: falha ao assinar algum topico\n");
    }

    (void)Main_PublishStatus(dMAIN_STATUS_ONLINE);
    (void)printf("[no3] Conectado: topicos assinados e ONLINE publicado.\n");
}

/*
 * @brief   Publica OFFLINE (desconexao limpa nao dispara o Last Will) e
 *          encerra a MqttWrapper.
 */
static void Main_BrokerDeinit(void)
{
    bool isConnected = false;

    if((Mqtt_Wrapper_IsConnected(&isConnected) == eMQTT_WRAPPER_RETURN_OK)
       && (isConnected == true))
    {
        (void)Main_PublishStatus(dMAIN_STATUS_OFFLINE);
        usleep(dMAIN_SHUTDOWN_FLUSH_US);
    }

    (void)Mqtt_Wrapper_Deinit();
}

/*
 * @brief   Thread auxiliar: em cada reconexao refaz assinaturas e ONLINE;
 *          em cada queda marca os outros nos como offline.
 * @param   argument - Nao utilizado
 * @retval  NULL
 */
static void *Main_WatchdogThread(void *argument)
{
    bool isConnected;

    (void)argument;

    while(mainNode3.stopRequested == 0)
    {
        isConnected = false;

        if(Mqtt_Wrapper_IsConnected(&isConnected) == eMQTT_WRAPPER_RETURN_OK)
        {
            if((isConnected == true) && (mainNode3.wasConnected == false))
            {
                Main_OnConnected();
            }
            else if((isConnected == false) && (mainNode3.wasConnected == true))
            {
                (void)printf("[no3] Conexao com o broker perdida (reconectando).\n");
                Fusion_OnBrokerDisconnected();
            }

            mainNode3.wasConnected = isConnected;
        }

        usleep(dMAIN_WATCHDOG_PERIOD_US);
    }

    return NULL;
}

int main(int argc, char **argv)
{
    int status = 1;
    const char *brokerHost = dMAIN_BROKER_HOST;
    int brokerPort = dMAIN_BROKER_PORT;

    if(argc > dMAIN_ARG_HOST)
    {
        brokerHost = argv[dMAIN_ARG_HOST];
    }

    if(argc > dMAIN_ARG_PORT)
    {
        brokerPort = atoi(argv[dMAIN_ARG_PORT]);
    }

    memset(&mainNode3, 0, sizeof(mainNode3));
    errno = 0;

    if(BspGpio_Init() == false)
    {
        // errno ajuda a diferenciar: EACCES/EPERM (permissao), EBUSY
        // (linha em uso por outro processo), ENOENT (gpiochip inexistente)
        (void)fprintf(stderr, "Falha ao iniciar o GPIO (errno=%d: %s)\n",
                      errno, strerror(errno));
    }
    else if(BspButton_Init() == false)
    {
        (void)fprintf(stderr, "Falha ao iniciar os botoes (errno=%d: %s)\n",
                      errno, strerror(errno));
    }
    else
    {
        Fusion_Init();

        if(Main_BrokerInit(brokerHost, brokerPort) == false)
        {
            (void)fprintf(stderr, "ERRO: Mqtt_Wrapper_Init falhou. O broker esta rodando?\n");
        }
        else if(Main_WaitForConnection() == false)
        {
            (void)fprintf(stderr, "ERRO: broker nao confirmou a conexao a tempo.\n");
            Main_BrokerDeinit();
        }
        else
        {
            Main_OnConnected();
            mainNode3.wasConnected = true;

            if(pthread_create(&mainNode3.watchdogThread, NULL, Main_WatchdogThread, NULL) != 0)
            {
                (void)fprintf(stderr, "Falha ao iniciar a thread de conexao\n");
            }
            else
            {
                status = BspScreen_Run(argc, argv);

                mainNode3.stopRequested = 1;
                (void)pthread_join(mainNode3.watchdogThread, NULL);
            }

            Main_BrokerDeinit();
        }
    }

    BspGpio_Deinit();

    return status;
}
