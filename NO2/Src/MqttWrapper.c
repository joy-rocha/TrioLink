/**************************************
* @file MqttWrapper.c
* @addtogroup MqttWrapper
* @brief Wrapper sobre libmosquitto para publish/subscribe MQTT com TLS e
         autenticacao usuario/senha, com reconexao automatica.
* @author Caio Oliveira
* @details
* \n <b>Ferramentas:</b>
* - Raspberry Pi 5, Linux, libmosquitto (pacote mosquitto).
* \n <b>Dependencias:</b>
* - AssertTypes.
* - libmosquitto (link com -lmosquitto).
* - pthread (link com -lpthread), usado apenas para o mutex interno; a
*   thread de rede em si e gerenciada pela propria libmosquitto via
*   mosquitto_loop_start().
* \n <b>Observacoes:</b>
* - Biblioteca singleton: suporta uma unica conexao MQTT por processo,
*   adequado ao projeto porque cada no roda um unico binario cliente.
* - mosquitto_loop_start() cria uma thread de rede interna da propria
*   libmosquitto: os callbacks desta biblioteca (conexao, mensagem
*   recebida) executam NESSA thread, nao na thread principal da
*   aplicacao. Por isso o estado interno (isConnected, ponteiro de
*   callback) e protegido por mutex.
* - Limitacao conhecida: em caso de reconexao apos queda, assinaturas
*   anteriores (Mqtt_Wrapper_Subscribe) nao sao refeitas automaticamente
*   pela biblioteca. Quem utiliza a wrapper deve chamar Subscribe
*   novamente dentro do fluxo de deteccao de reconexao, caso isso seja
*   necessario para o caso de uso.
* @copyright https://github.com/joy-rocha/TrioLink
**************************************/

/**************************************
* INCLUDES
**************************************/
#include "MqttWrapper.h"
#include <mosquitto.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

/**************************************
* DEFINES LOCAIS
**************************************/
/* Nao ha defines locais nesta biblioteca */

/**************************************
* CONSTANTES
**************************************/
/* Nao ha constantes internas nesta biblioteca */

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
**************************************/
/// Variaveis internas da biblioteca (conexao MQTT e sincronizacao entre
/// a thread de rede da libmosquitto e a thread principal da aplicacao)
static struct
{
    /// Handle da conexao mantido pela libmosquitto
    struct mosquitto *mosq;
    /// Funcao de callback fornecida pelo usuario em Mqtt_Wrapper_Init()
    mqttWrapperMessageCallback_t messageCallback;
    /// Protege isConnected e messageCallback contra acesso simultaneo
    /// pela thread de rede (callbacks) e pela thread principal
    pthread_mutex_t lock;
    /// Indica se Mqtt_Wrapper_Init() ja foi chamada com sucesso
    bool isInitialized;
    /// Indica se a conexao com o broker esta ativa no momento
    bool isConnected;
} mqttWrapper;

/**************************************
* PROTOTIPOS LOCAIS
**************************************/
static int Mqtt_Wrapper_QosToInt(mqttWrapperQos_t qos);
static void Mqtt_Wrapper_OnConnect(struct mosquitto *mosq, void *userData, int resultCode);
static void Mqtt_Wrapper_OnDisconnect(struct mosquitto *mosq, void *userData, int resultCode);
static void Mqtt_Wrapper_OnMessage(struct mosquitto *mosq, void *userData, const struct mosquitto_message *message);

/**************************************
* FUNCOES PUBLICAS
**************************************/
/**************************************
/** @brief Inicializa a biblioteca libmosquitto, configura TLS, credenciais
           e Last Will, conecta ao broker e inicia a thread de rede.
* @param config: ponteiro para a estrutura de configuracao preenchida pelo
                 chamador. Nao e necessario manter este ponteiro valido
                 apos o retorno da funcao: os dados sao copiados.
* @retval eMQTT_WRAPPER_RETURN_OK se a conexao foi estabelecida com sucesso.
          eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT se config for dNULL.
          eMQTT_WRAPPER_RETURN_ALREADY_INITIALIZED se chamada mais de uma vez.
          eMQTT_WRAPPER_RETURN_CONNECTION_ERROR se a conexao inicial falhar.
* @details Apos retornar eMQTT_WRAPPER_RETURN_OK, a reconexao em caso de
           queda e automatica (gerenciada pela thread interna da
           libmosquitto), com atraso crescente entre tentativas (ver
           dMQTT_WRAPPER_RECONNECT_DELAY_MIN_SECONDS e
           dMQTT_WRAPPER_RECONNECT_DELAY_MAX_SECONDS).
**************************************/
mqttWrapperReturn_t Mqtt_Wrapper_Init(const mqttWrapperConfig_t *config)
{
    mqttWrapperReturn_t result = eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT;

    if(config == dNULL)
    {
        result = eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT;
    }
    else if(mqttWrapper.isInitialized == true)
    {
        result = eMQTT_WRAPPER_RETURN_ALREADY_INITIALIZED;
    }
    else
    {
        memset(&mqttWrapper, 0, sizeof(mqttWrapper));
        pthread_mutex_init(&mqttWrapper.lock, dNULL);
        mqttWrapper.messageCallback = config->messageCallback;

        mosquitto_lib_init();

        mqttWrapper.mosq = mosquitto_new(config->clientId, true, dNULL);

        if(mqttWrapper.mosq == dNULL)
        {
            result = eMQTT_WRAPPER_RETURN_CONNECTION_ERROR;
        }
        else
        {
            // Usuario e senha, se configurados (string vazia desabilita)
            if(strlen(config->username) > 0)
            {
                mosquitto_username_pw_set(mqttWrapper.mosq, config->username, config->password);
            }

            // TLS, se habilitado
            if(config->useTls == true)
            {
                mosquitto_tls_set(mqttWrapper.mosq, config->caFilePath, dNULL, dNULL, dNULL, dNULL);
            }

            // Last Will, configurado antes do connect: o broker precisa
            // saber o testamento deste cliente desde o pacote CONNECT
            if(strlen(config->willTopic) > 0)
            {
                mosquitto_will_set(mqttWrapper.mosq, config->willTopic,
                                    (int)strlen(config->willPayload), config->willPayload,
                                    Mqtt_Wrapper_QosToInt(config->willQos), config->willRetain);
            }

            mosquitto_connect_callback_set(mqttWrapper.mosq, Mqtt_Wrapper_OnConnect);
            mosquitto_disconnect_callback_set(mqttWrapper.mosq, Mqtt_Wrapper_OnDisconnect);
            mosquitto_message_callback_set(mqttWrapper.mosq, Mqtt_Wrapper_OnMessage);

            // Define o atraso entre tentativas de reconexao automatica,
            // crescendo exponencialmente ate o maximo configurado
            mosquitto_reconnect_delay_set(mqttWrapper.mosq,
                                           dMQTT_WRAPPER_RECONNECT_DELAY_MIN_SECONDS,
                                           dMQTT_WRAPPER_RECONNECT_DELAY_MAX_SECONDS, true);

            if(mosquitto_connect(mqttWrapper.mosq, config->brokerHost, (int)config->brokerPort, dMQTT_WRAPPER_KEEPALIVE_SECONDS) != MOSQ_ERR_SUCCESS)
            {
                result = eMQTT_WRAPPER_RETURN_CONNECTION_ERROR;
            }
            else if(mosquitto_loop_start(mqttWrapper.mosq) != MOSQ_ERR_SUCCESS)
            {
                result = eMQTT_WRAPPER_RETURN_CONNECTION_ERROR;
            }
            else
            {
                mqttWrapper.isInitialized = true;
                result = eMQTT_WRAPPER_RETURN_OK;
            }
        }
    }
    return result;
}

/**************************************
/** @brief Publica uma mensagem em um topico.
* @param topic: nome do topico.
* @param payload: conteudo a ser publicado.
* @param payloadLen: tamanho de payload, em bytes.
* @param qos: nivel de QoS da publicacao.
* @param retain: se true, o broker guarda esta mensagem como a ultima
                 conhecida deste topico, entregue a novos assinantes.
* @retval eMQTT_WRAPPER_RETURN_OK se a mensagem foi entregue a libmosquitto
          para envio (nao garante que o broker ja a recebeu; QoS 1/2 fazem
          essa garantia de forma assincrona, via a propria biblioteca).
          eMQTT_WRAPPER_RETURN_NOT_INITIALIZED se Init() nao foi chamada.
          eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT se algum parametro for invalido.
          eMQTT_WRAPPER_RETURN_PUBLISH_ERROR em caso de falha da libmosquitto.
**************************************/
mqttWrapperReturn_t Mqtt_Wrapper_Publish(const char *topic, const char *payload, u32 payloadLen, mqttWrapperQos_t qos, bool retain)
{
    mqttWrapperReturn_t result = eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT;

    if((topic == dNULL) || (payload == dNULL))
    {
        result = eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT;
    }
    else if(mqttWrapper.isInitialized == false)
    {
        result = eMQTT_WRAPPER_RETURN_NOT_INITIALIZED;
    }
    else
    {
        int publishResult = mosquitto_publish(mqttWrapper.mosq, dNULL, topic,
                                               (int)payloadLen, payload,
                                               Mqtt_Wrapper_QosToInt(qos), retain);

        if(publishResult == MOSQ_ERR_SUCCESS)
        {
            result = eMQTT_WRAPPER_RETURN_OK;
        }
        else
        {
            result = eMQTT_WRAPPER_RETURN_PUBLISH_ERROR;
        }
    }
    return result;
}

/**************************************
/** @brief Assina um topico, passando a receber suas mensagens via o
           callback configurado em Mqtt_Wrapper_Init().
* @param topic: nome do topico, podendo conter curingas ("+", "#").
* @param qos: nivel de QoS desejado para a assinatura.
* @retval eMQTT_WRAPPER_RETURN_OK se a assinatura foi solicitada com sucesso.
          eMQTT_WRAPPER_RETURN_NOT_INITIALIZED se Init() nao foi chamada.
          eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT se topic for dNULL.
          eMQTT_WRAPPER_RETURN_SUBSCRIBE_ERROR em caso de falha da libmosquitto.
**************************************/
mqttWrapperReturn_t Mqtt_Wrapper_Subscribe(const char *topic, mqttWrapperQos_t qos)
{
    mqttWrapperReturn_t result = eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT;

    if(topic == dNULL)
    {
        result = eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT;
    }
    else if(mqttWrapper.isInitialized == false)
    {
        result = eMQTT_WRAPPER_RETURN_NOT_INITIALIZED;
    }
    else
    {
        int subscribeResult = mosquitto_subscribe(mqttWrapper.mosq, dNULL, topic, Mqtt_Wrapper_QosToInt(qos));

        if(subscribeResult == MOSQ_ERR_SUCCESS)
        {
            result = eMQTT_WRAPPER_RETURN_OK;
        }
        else
        {
            result = eMQTT_WRAPPER_RETURN_SUBSCRIBE_ERROR;
        }
    }
    return result;
}

/**************************************
/** @brief Informa se a conexao com o broker esta ativa neste instante.
* @param isConnected: ponteiro onde o estado sera escrito.
* @retval eMQTT_WRAPPER_RETURN_OK se a leitura foi realizada.
          eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT se isConnected for dNULL.
          eMQTT_WRAPPER_RETURN_NOT_INITIALIZED se Init() nao foi chamada.
**************************************/
mqttWrapperReturn_t Mqtt_Wrapper_IsConnected(bool *isConnected)
{
    mqttWrapperReturn_t result = eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT;

    if(isConnected == dNULL)
    {
        result = eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT;
    }
    else if(mqttWrapper.isInitialized == false)
    {
        result = eMQTT_WRAPPER_RETURN_NOT_INITIALIZED;
    }
    else
    {
        // Protegido por mutex: isConnected tambem e escrito pela thread de rede,
        // dentro de Mqtt_Wrapper_OnConnect/OnDisconnect
        pthread_mutex_lock(&mqttWrapper.lock);
        *isConnected = mqttWrapper.isConnected;
        pthread_mutex_unlock(&mqttWrapper.lock);

        result = eMQTT_WRAPPER_RETURN_OK;
    }
    return result;
}

/**************************************
/** @brief Encerra a conexao, para a thread de rede e libera os recursos da libmosquitto.
* @param Nenhum.
* @retval eMQTT_WRAPPER_RETURN_OK se finalizado com sucesso.
          eMQTT_WRAPPER_RETURN_NOT_INITIALIZED se Init() nao foi chamada.
**************************************/
mqttWrapperReturn_t Mqtt_Wrapper_Deinit(void)
{
    mqttWrapperReturn_t result = eMQTT_WRAPPER_RETURN_NOT_INITIALIZED;

    if(mqttWrapper.isInitialized == true)
    {
        mosquitto_disconnect(mqttWrapper.mosq);
        mosquitto_loop_stop(mqttWrapper.mosq, false);
        mosquitto_destroy(mqttWrapper.mosq);
        mosquitto_lib_cleanup();
        pthread_mutex_destroy(&mqttWrapper.lock);

        memset(&mqttWrapper, 0, sizeof(mqttWrapper));

        result = eMQTT_WRAPPER_RETURN_OK;
    }
    return result;
}

/**************************************
* FUNCOES LOCAIS
**************************************/
/**************************************
/** @brief Converte o enum publico de QoS para o inteiro esperado pela API
           da libmosquitto.
* @param qos: nivel de QoS no formato desta biblioteca.
* @retval O valor inteiro correspondente (0, 1 ou 2).
**************************************/
static int Mqtt_Wrapper_QosToInt(mqttWrapperQos_t qos)
{
    int qosInt;

    if(qos == eMQTT_WRAPPER_QOS_1)
    {
        qosInt = 1;
    }
    else if(qos == eMQTT_WRAPPER_QOS_2)
    {
        qosInt = 2;
    }
    else
    {
        qosInt = 0;
    }
    return qosInt;
}

/**************************************
/** @brief Callback da libmosquitto, chamado na thread de rede interna
           sempre que a conexao com o broker e estabelecida (incluindo
           reconexoes automaticas).
* @param mosq: handle da conexao (nao utilizado).
* @param userData: dado de usuario passado em mosquitto_new (nao utilizado).
* @param resultCode: 0 indica sucesso; qualquer outro valor indica falha.
**************************************/
static void Mqtt_Wrapper_OnConnect(struct mosquitto *mosq, void *userData, int resultCode)
{
    (void)mosq;
    (void)userData;

    pthread_mutex_lock(&mqttWrapper.lock);
    mqttWrapper.isConnected = (resultCode == 0);
    pthread_mutex_unlock(&mqttWrapper.lock);
}

/**************************************
/** @brief Callback da libmosquitto, chamado na thread de rede interna
           sempre que a conexao com o broker e perdida.
* @param mosq: handle da conexao (nao utilizado).
* @param userData: dado de usuario passado em mosquitto_new (nao utilizado).
* @param resultCode: motivo da desconexao, reportado pela libmosquitto.
**************************************/
static void Mqtt_Wrapper_OnDisconnect(struct mosquitto *mosq, void *userData, int resultCode)
{
    (void)mosq;
    (void)userData;
    (void)resultCode;

    pthread_mutex_lock(&mqttWrapper.lock);
    mqttWrapper.isConnected = false;
    pthread_mutex_unlock(&mqttWrapper.lock);
}

/**************************************
/** @brief Callback da libmosquitto, chamado na thread de rede interna
           sempre que uma mensagem chega em um topico assinado. Repassa a
           mensagem para o callback do usuario configurado em Init().
* @param mosq: handle da conexao (nao utilizado).
* @param userData: dado de usuario passado em mosquitto_new (nao utilizado).
* @param message: estrutura da libmosquitto contendo topico e payload.
**************************************/
static void Mqtt_Wrapper_OnMessage(struct mosquitto *mosq, void *userData, const struct mosquitto_message *message)
{
    (void)mosq;
    (void)userData;

    mqttWrapperMessageCallback_t callbackToCall = dNULL;

    pthread_mutex_lock(&mqttWrapper.lock);
    callbackToCall = mqttWrapper.messageCallback;
    pthread_mutex_unlock(&mqttWrapper.lock);

    if((callbackToCall != dNULL) && (message->payload != dNULL))
    {
        callbackToCall(message->topic, (const char *)message->payload, (u32)message->payloadlen);
    }
}
/** @} DOXYGEN GROUP TAG END OF FILE */
