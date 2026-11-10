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
 * @brief   Cria o cliente MQTT, assina os topicos e inicia a thread de rede.
 *          Se o broker estiver fora do ar, a conexao e tentada novamente
 *          em segundo plano.
 * @param   brokerHost - Endereco do broker (ex: "localhost")
 * @param   brokerPort - Porta do broker (ex: 1883)
 * @retval  true  - Cliente criado e thread iniciada
 * @retval  false - Parametro invalido ou falha ao criar o cliente
 */
bool Fusion_Init(const char *brokerHost, int brokerPort);

/*
 * @brief   Publica OFFLINE, desconecta do broker e libera os recursos.
 * @param   void
 * @retval  void
 */
void Fusion_Deinit(void);

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