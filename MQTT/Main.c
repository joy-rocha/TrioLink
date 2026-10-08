/**************************************
* @file Main.c
* @brief Programa de teste da MqttWrapper: publica um contador fake a cada
         2 segundos e imprime tudo que receber.
* @details
* \n <b>Observacoes:</b>
* - Conexao ANONIMA e SEM TLS, apenas para teste local. Nao usar assim no
*   projeto final.
* - O programa assina o mesmo topico em que publica, entao voce vera cada
*   mensagem enviada voltando pelo callback (prova que Publish, broker,
*   Subscribe e OnMessage estao funcionando de ponta a ponta).
* - Encerrar com Ctrl+C (desconexao limpa, o Last Will NAO e disparado).
**************************************/

/**************************************
* INCLUDES
**************************************/
#include "MqttWrapper.h"
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**************************************
* DEFINES LOCAIS
**************************************/
/// Endereco do broker (localhost se o Mosquitto roda nesta mesma maquina)
#define dTEST_BROKER_HOST            "localhost"
/// Porta do broker sem TLS
#define dTEST_BROKER_PORT            1883
/// Identificador deste cliente. Deve ser unico por conexao no broker.
#define dTEST_CLIENT_ID              "teste_contador"
/// Topico onde o contador e publicado (e tambem assinado, para ver o retorno)
#define dTEST_TOPIC_COUNTER          "teste/contador"
/// Topico do Last Will, publicado pelo broker se o programa cair sem avisar
#define dTEST_TOPIC_STATUS           "teste/status"
/// Intervalo entre publicacoes
#define dTEST_PUBLISH_PERIOD_SECONDS 2    // [s]
/// Tempo maximo esperando a conexao ser confirmada pelo broker
#define dTEST_CONNECT_TIMEOUT_MS     5000 // [ms]
/// Intervalo entre checagens enquanto espera a conexao
#define dTEST_CONNECT_POLL_MS        100  // [ms]

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
**************************************/
/// Variaveis internas do programa de teste
static struct
{
    /// Setada pelo handler de SIGINT (Ctrl+C) para encerrar o loop principal
    volatile sig_atomic_t stopRequested;
} mainTest;

/**************************************
* PROTOTIPOS LOCAIS
**************************************/
static void Main_SignalHandler(int signalNumber);
static void Main_OnMessageReceived(const char *topic, const char *payload, u32 payloadLen);
static bool Main_WaitForConnection(void);

/**************************************
* FUNCAO PRINCIPAL
**************************************/
int main(void)
{
    int returnCode = 0;
    mqttWrapperConfig_t config;
    u32 counter = 0;

    memset(&config, 0, sizeof(config));
    mainTest.stopRequested = 0;

    // Ctrl+C apenas levanta uma flag; o encerramento de verdade e feito
    // pelo loop principal, fora do contexto do handler
    signal(SIGINT, Main_SignalHandler);

    // Configuracao: anonima, sem TLS
    strcpy(config.brokerHost, dTEST_BROKER_HOST);
    config.brokerPort = dTEST_BROKER_PORT;
    strcpy(config.clientId, dTEST_CLIENT_ID);
    config.username[0] = '\0';
    config.password[0] = '\0';
    config.useTls = false;
    strcpy(config.willTopic, dTEST_TOPIC_STATUS);
    strcpy(config.willPayload, "OFFLINE");
    config.willQos = eMQTT_WRAPPER_QOS_1;
    config.willRetain = true;
    config.messageCallback = Main_OnMessageReceived;

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
        printf("[main] Conectado. Assinando %s\n", dTEST_TOPIC_COUNTER);

        if(Mqtt_Wrapper_Subscribe(dTEST_TOPIC_COUNTER, eMQTT_WRAPPER_QOS_0) != eMQTT_WRAPPER_RETURN_OK)
        {
            fprintf(stderr, "[main] AVISO: falha ao assinar %s\n", dTEST_TOPIC_COUNTER);
        }

        // Publica o status como ONLINE (retido). Se o programa cair sem
        // avisar, o broker troca isso por OFFLINE via Last Will.
        Mqtt_Wrapper_Publish(dTEST_TOPIC_STATUS, "ONLINE", 6, eMQTT_WRAPPER_QOS_1, true);

        printf("[main] Publicando a cada %d s. Ctrl+C para sair.\n", dTEST_PUBLISH_PERIOD_SECONDS);

        while(mainTest.stopRequested == 0)
        {
            char payload[64];
            bool isConnected = false;
            int payloadLen;

            counter++;
            payloadLen = snprintf(payload, sizeof(payload), "{\"contador\":%u}", counter);

            Mqtt_Wrapper_IsConnected(&isConnected);

            if(Mqtt_Wrapper_Publish(dTEST_TOPIC_COUNTER, payload, (u32)payloadLen, eMQTT_WRAPPER_QOS_0, false) == eMQTT_WRAPPER_RETURN_OK)
            {
                printf("[main] Publicado #%u (conectado=%s)\n", counter, (isConnected == true) ? "sim" : "nao");
            }
            else
            {
                printf("[main] FALHA ao publicar #%u (conectado=%s)\n", counter, (isConnected == true) ? "sim" : "nao");
            }

            // sleep() e interrompido pelo Ctrl+C, entao a saida e imediata
            sleep(dTEST_PUBLISH_PERIOD_SECONDS);
        }

        printf("\n[main] Encerrando...\n");

        // Desconexao limpa NAO dispara o Last Will, entao avisamos OFFLINE
        // manualmente. O Last Will so cobre quedas abruptas (crash, rede).
        Mqtt_Wrapper_Publish(dTEST_TOPIC_STATUS, "OFFLINE", 7, eMQTT_WRAPPER_QOS_1, true);
        Mqtt_Wrapper_Deinit();
    }
    return returnCode;
}

/**************************************
* FUNCOES LOCAIS
**************************************/
/**************************************
/** @brief Handler de SIGINT (Ctrl+C). Apenas sinaliza o encerramento.
* @param signalNumber: numero do sinal recebido (nao utilizado).
**************************************/
static void Main_SignalHandler(int signalNumber)
{
    (void)signalNumber;
    mainTest.stopRequested = 1;
}

/**************************************
/** @brief Callback chamado pela wrapper a cada mensagem recebida. Executa
           na THREAD DE REDE da libmosquitto, nao na thread do main.
* @param topic: topico da mensagem.
* @param payload: conteudo (NAO e garantido terminar em zero).
* @param payloadLen: tamanho do conteudo, em bytes.
**************************************/
static void Main_OnMessageReceived(const char *topic, const char *payload, u32 payloadLen)
{
    // %.*s imprime exatamente payloadLen caracteres, sem depender de '\0'
    printf("[callback] Recebido em '%s': %.*s\n", topic, (int)payloadLen, payload);
}

/**************************************
/** @brief Espera o broker confirmar a conexao (CONNACK). Init() retorna
           assim que o pedido e enviado, mas isConnected so vira true
           depois que o OnConnect for chamado pela thread de rede.
* @param Nenhum.
* @retval true se conectou dentro do tempo limite; false caso contrario.
**************************************/
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
