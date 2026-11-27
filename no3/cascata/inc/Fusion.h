/*
 * @file    Fusion.h
 * @brief   Recebe os dados MQTT dos outros nos (BMP e MPU), valida o JSON,
 *          controla a obsolescencia (staleness) e classifica os dados como
 *          normais ou anormais.
 */

#ifndef FUSION_H
#define FUSION_H

#include <stdbool.h>
#include <stdint.h>

/// Classificacao dos valores de um sensor
typedef enum
{
    eFUSION_HEALTH_NORMAL = 0,
    eFUSION_HEALTH_ABNORMAL
} fusionHealth_t;

/// Ultimos dados do sensor BMP
typedef struct
{
    bool hasData;               // true se ja chegou ao menos uma leitura valida
    bool connected;             // true se o status do no 1 for ONLINE
    bool stale;                 // true se os dados estao obsoletos
    fusionHealth_t health;      // Classificacao normal / anormal
    int64_t timestamp;          // Instante da leitura [ms Unix UTC]
    float temperature;          // [°C]
    float pressure;             // [Pa]
    float altitude;             // [m]
} fusionBmp_t;

/// Ultimos dados do sensor MPU
typedef struct
{
    bool hasData;               // true se ja chegou ao menos uma leitura valida
    bool connected;             // true se o status do no 2 for ONLINE
    bool stale;                 // true se os dados estao obsoletos
    fusionHealth_t health;      // Classificacao normal / anormal
    int64_t timestamp;          // Instante da leitura [ms Unix UTC]
    float accelX;               // [m/s²]
    float accelY;               // [m/s²]
    float accelZ;               // [m/s²]
    float gyroX;                // [rad/s]
    float gyroY;                // [rad/s]
    float gyroZ;                // [rad/s]
} fusionMpu_t;

/*
 * @brief   Zera o estado dos sensores. Nao cria conexao MQTT: a conexao e
 *          da MqttWrapper (ver Main.c), que entrega as mensagens em
 *          Fusion_OnMessage().
 * @param   void
 * @retval  void
 */
void Fusion_Init(void);

/*
 * @brief   Assina os topicos de dados e de status dos outros nos usando a
 *          MqttWrapper. Deve ser chamada a cada (re)conexao com o broker,
 *          pois a sessao e limpa (clean_session) e as assinaturas se perdem.
 * @param   void
 * @retval  true  - Todas as assinaturas foram solicitadas
 * @retval  false - Alguma assinatura falhou
 */
bool Fusion_Subscribe(void);

/*
 * @brief   Callback de mensagens (mesma assinatura da MqttWrapper): direciona
 *          o payload ao tratador do topico. Roda na thread de rede.
 * @param   topic      - Topico da mensagem
 * @param   payload    - Conteudo (nao e garantido terminar em zero)
 * @param   payloadLen - Tamanho do conteudo [bytes]
 * @retval  void
 */
void Fusion_OnMessage(const char *topic, const char *payload, uint32_t payloadLen);

/*
 * @brief   Informa que a conexao com o broker caiu: sem broker nao ha como
 *          saber o status dos outros nos, entao ambos voltam para offline.
 * @param   void
 * @retval  void
 */
void Fusion_OnBrokerDisconnected(void);

/*
 * @brief   Copia o estado atual do sensor BMP (thread-safe).
 * @param   data - Ponteiro que recebe os dados
 * @retval  true  - Copia realizada
 * @retval  false - Ponteiro nulo
 */
bool Fusion_GetBmp(fusionBmp_t *data);

/*
 * @brief   Copia o estado atual do sensor MPU (thread-safe).
 * @param   data - Ponteiro que recebe os dados
 * @retval  true  - Copia realizada
 * @retval  false - Ponteiro nulo
 */
bool Fusion_GetMpu(fusionMpu_t *data);

#endif /* FUSION_H */