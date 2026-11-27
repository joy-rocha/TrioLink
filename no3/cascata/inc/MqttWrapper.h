/**************************************
* @file MqttWrapper.h
* @addtogroup MqttWrapper
* @{
**************************************/
#ifndef _MQTT_WRAPPER_H_
#define _MQTT_WRAPPER_H_

/**************************************
* INCLUDES NECESSARIOS
**************************************/
#include "Asserttypes.h"

/**************************************
* CONFIGURACOES
**************************************/
/// Intervalo maximo (segundos) sem trafego antes do broker considerar a
/// conexao morta e disparar o Last Will deste cliente
#define dMQTT_WRAPPER_KEEPALIVE_SECONDS            60  // [s]

/// Menor atraso entre tentativas de reconexao automatica
#define dMQTT_WRAPPER_RECONNECT_DELAY_MIN_SECONDS  2   // [s]

/// Maior atraso entre tentativas de reconexao automatica (o atraso cresce
/// exponencialmente a cada falha, ate este limite)
#define dMQTT_WRAPPER_RECONNECT_DELAY_MAX_SECONDS  30  // [s]

/**************************************
* DEFINES PUBLICOS
**************************************/
/// Tamanho maximo de um nome de topico
#define dMQTT_WRAPPER_MAX_TOPIC_LEN     128 // [bytes]
/// Tamanho maximo de um payload publicado ou recebido
#define dMQTT_WRAPPER_MAX_PAYLOAD_LEN   2048 // [bytes]
/// Tamanho maximo dos campos de configuracao de conexao (host, client id,
/// usuario, senha, caminhos de arquivo)
#define dMQTT_WRAPPER_MAX_FIELD_LEN     128 // [bytes]

/**************************************
* TIPOS DE DADOS PUBLICOS
**************************************/
/// Possiveis retornos das funcoes desta biblioteca
typedef enum
{
    eMQTT_WRAPPER_RETURN_OK,
    eMQTT_WRAPPER_RETURN_INVALID_ARGUMENT,
    eMQTT_WRAPPER_RETURN_NOT_INITIALIZED,
    eMQTT_WRAPPER_RETURN_ALREADY_INITIALIZED,
    eMQTT_WRAPPER_RETURN_CONNECTION_ERROR,
    eMQTT_WRAPPER_RETURN_PUBLISH_ERROR,
    eMQTT_WRAPPER_RETURN_SUBSCRIBE_ERROR,

    eMQTT_WRAPPER_RETURN_END_ENUM
} mqttWrapperReturn_t;

/// Niveis de QoS suportados
typedef enum
{
    eMQTT_WRAPPER_QOS_0,
    eMQTT_WRAPPER_QOS_1,
    eMQTT_WRAPPER_QOS_2,

    eMQTT_WRAPPER_QOS_END_ENUM
} mqttWrapperQos_t;

/// Assinatura da funcao de callback chamada sempre que uma mensagem chega em algum topico assinado. 
/// Executada na thread de rede interna da biblioteca: nao deve bloquear por muito tempo,
/// e qualquer acesso a dados compartilhados com a thread principal da aplicacao deve ser
/// protegido pelo proprio chamador (ex.: mutex, variavel atomica).
/// @param topic: nome do topico onde a mensagem chegou (string terminada em zero).
/// @param payload: conteudo da mensagem. Nao e garantido ser terminado em zero.
/// @param payloadLen: tamanho de payload, em bytes.
typedef void (*mqttWrapperMessageCallback_t)(const char *topic, const char *payload, u32 payloadLen);

/// Parametros de configuracao passados a Mqtt_Wrapper_Init(). 
/// Cada no do projeto preenche esta estrutura com seus proprios dados de conexao.
typedef struct
{
    /// Endereco (IP ou hostname) do broker
    char brokerHost[dMQTT_WRAPPER_MAX_FIELD_LEN];
    /// Porta do broker (8883 para TLS, 1883 para conexao sem criptografia)
    u16 brokerPort;
    /// Identificador unico deste cliente perante o broker
    char clientId[dMQTT_WRAPPER_MAX_FIELD_LEN];
    /// Usuario para autenticacao. Vazio ("") se nao utilizado.
    char username[dMQTT_WRAPPER_MAX_FIELD_LEN];
    /// Senha para autenticacao. Vazio ("") se nao utilizado.
    char password[dMQTT_WRAPPER_MAX_FIELD_LEN];
    /// Habilita (true) ou desabilita (false) TLS
    bool useTls;
    /// Caminho do certificado da CA, usado para validar o broker quando
    /// useTls for true. Ignorado se useTls for false.
    char caFilePath[dMQTT_WRAPPER_MAX_FIELD_LEN];
    /// Topico do Last Will deste cliente. Vazio ("") para nao configurar LWT.
    char willTopic[dMQTT_WRAPPER_MAX_TOPIC_LEN];
    /// Payload do Last Will, publicado automaticamente pelo broker caso
    /// este cliente caia sem se desconectar de forma limpa
    char willPayload[dMQTT_WRAPPER_MAX_PAYLOAD_LEN];
    /// QoS do Last Will
    mqttWrapperQos_t willQos;
    /// Define se o Last Will deve ser publicado com retain
    bool willRetain;
    /// Funcao chamada a cada mensagem recebida em um topico assinado.
    /// Pode ser dNULL se este cliente apenas publica, sem assinar nada.
    mqttWrapperMessageCallback_t messageCallback;
} mqttWrapperConfig_t;

/**************************************
* PROTOTIPOS PUBLICOS
**************************************/
mqttWrapperReturn_t Mqtt_Wrapper_Init(const mqttWrapperConfig_t *config);
mqttWrapperReturn_t Mqtt_Wrapper_Publish(const char *topic, const char *payload, u32 payloadLen, mqttWrapperQos_t qos, bool retain);
mqttWrapperReturn_t Mqtt_Wrapper_Subscribe(const char *topic, mqttWrapperQos_t qos);
mqttWrapperReturn_t Mqtt_Wrapper_IsConnected(bool *isConnected);
mqttWrapperReturn_t Mqtt_Wrapper_Deinit(void);

#endif /* _MQTT_WRAPPER_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
