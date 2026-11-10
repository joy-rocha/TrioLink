/*
 * @file    Fusion.c
 * @brief   Implementacao do recebimento MQTT, parse cJSON, staleness e
 *          analise dos dados dos sensores.
 */

#include "Fusion.h"

#include <cjson/cJSON.h>
#include <math.h>
#include <mosquitto.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/// Identificador do cliente MQTT deste no
#define dFUSION_CLIENT_ID               "rasp3-display"

/// Intervalo de keep-alive da conexao MQTT [s]
#define dFUSION_KEEPALIVE_S             30

/// Topicos assinados e publicados
#define dFUSION_TOPIC_BMP_RAW           "sensor/bmp/raw"
#define dFUSION_TOPIC_MPU_FUSION        "fusion/bmp_mpu"
#define dFUSION_TOPIC_STATUS_RASP1      "status/rasp1"
#define dFUSION_TOPIC_STATUS_RASP2      "status/rasp2"
#define dFUSION_TOPIC_STATUS_RASP3      "status/rasp3"

/// Valores do campo Status
#define dFUSION_STATUS_ONLINE           "ONLINE"
#define dFUSION_STATUS_OFFLINE          "OFFLINE"

/// Mensagem de ultima vontade (Last Will), publicada pelo broker se o no cair
#define dFUSION_WILL_PAYLOAD            "{\"timestamp\":0,\"Status\":\"OFFLINE\"}"

/// Intervalo de publicacao esperado de cada topico [ms] (combinar com os nos)
#define dFUSION_BMP_INTERVAL_MS         1000
#define dFUSION_MPU_INTERVAL_MS         1000

/// Dado e obsoleto apos este multiplo do intervalo esperado
#define dFUSION_STALENESS_FACTOR        3

/// Faixas consideradas normais para ambiente comum
#define dFUSION_BMP_TEMP_MIN            -10.0f      // [°C]
#define dFUSION_BMP_TEMP_MAX            50.0f       // [°C]
#define dFUSION_BMP_PRESSURE_MIN        80000.0f    // [Pa]
#define dFUSION_BMP_PRESSURE_MAX        110000.0f   // [Pa]
#define dFUSION_MPU_ACCEL_MAX           20.0f       // Modulo maximo [m/s²]
#define dFUSION_MPU_GYRO_MAX            10.0f       // Modulo maximo [rad/s]

/// Conversoes de tempo
#define dFUSION_MS_PER_SECOND           1000U
#define dFUSION_NS_PER_MS               1000000U

/// Dados locais do modulo
static struct
{
    struct mosquitto *client;
    pthread_mutex_t mutex;
    bool initialized;

    struct
    {
        bool valid;
        bool online;
        uint64_t lastRxMs;
        fusionBmp_t data;
    } bmp;

    struct
    {
        bool valid;
        bool online;
        uint64_t lastRxMs;
        fusionMpu_t data;
    } mpu;
} fusion = { .mutex = PTHREAD_MUTEX_INITIALIZER };

/*
 * @brief   Le um relogio do sistema em milissegundos.
 * @param   clockId - CLOCK_MONOTONIC (staleness) ou CLOCK_REALTIME (timestamp)
 * @retval  Tempo atual [ms]
 */
static uint64_t Fusion_GetClockMs(clockid_t clockId)
{
    struct timespec now;

    clock_gettime(clockId, &now);

    return ((uint64_t)now.tv_sec * dFUSION_MS_PER_SECOND)
           + ((uint64_t)now.tv_nsec / dFUSION_NS_PER_MS);
}

/*
 * @brief   Verifica se o dado esta obsoleto.
 * @param   valid      - true se ja houve alguma leitura valida
 * @param   lastRxMs   - Instante da ultima mensagem recebida [ms, monotonico]
 * @param   nowMs      - Instante atual [ms, monotonico]
 * @param   intervalMs - Intervalo de publicacao esperado [ms]
 * @retval  true  - Dado obsoleto (ou inexistente)
 * @retval  false - Dado dentro do prazo
 */
static bool Fusion_IsStale(bool valid,
                           uint64_t lastRxMs,
                           uint64_t nowMs,
                           uint32_t intervalMs)
{
    if(valid == false)
    {
        return true;
    }

    return ((nowMs - lastRxMs) > ((uint64_t)intervalMs * dFUSION_STALENESS_FACTOR));
}

/*
 * @brief   Classifica os valores do BMP como normais ou anormais.
 * @param   temperature - Temperatura [°C]
 * @param   pressure    - Pressao [Pa]
 * @param   altitude    - Altitude [m]
 * @retval  Classificacao dos valores
 */
static fusionHealth_t Fusion_EvaluateBmp(float temperature,
                                         float pressure,
                                         float altitude)
{
    bool normal = ((isfinite(temperature) != 0)
                   && (isfinite(pressure) != 0)
                   && (isfinite(altitude) != 0)
                   && (temperature >= dFUSION_BMP_TEMP_MIN)
                   && (temperature <= dFUSION_BMP_TEMP_MAX)
                   && (pressure >= dFUSION_BMP_PRESSURE_MIN)
                   && (pressure <= dFUSION_BMP_PRESSURE_MAX));

    return (normal == true) ? eFUSION_HEALTH_NORMAL : eFUSION_HEALTH_ABNORMAL;
}

/*
 * @brief   Classifica os valores do MPU como normais ou anormais.
 *          Compara os modulos ao quadrado para nao depender da libm.
 * @param   data - Dados do MPU ja preenchidos
 * @retval  Classificacao dos valores
 */
static fusionHealth_t Fusion_EvaluateMpu(const fusionMpu_t *data)
{
    float accelSq = (data->accelX * data->accelX)
                    + (data->accelY * data->accelY)
                    + (data->accelZ * data->accelZ);
    float gyroSq = (data->gyroX * data->gyroX)
                   + (data->gyroY * data->gyroY)
                   + (data->gyroZ * data->gyroZ);
    bool normal = ((isfinite(accelSq) != 0)
                   && (isfinite(gyroSq) != 0)
                   && (accelSq <= (dFUSION_MPU_ACCEL_MAX * dFUSION_MPU_ACCEL_MAX))
                   && (gyroSq <= (dFUSION_MPU_GYRO_MAX * dFUSION_MPU_GYRO_MAX)));

    return (normal == true) ? eFUSION_HEALTH_NORMAL : eFUSION_HEALTH_ABNORMAL;
}

/*
 * @brief   Le um campo numerico de um objeto cJSON.
 * @param   parent - Objeto que contem o campo
 * @param   key    - Nome do campo
 * @param   value  - Ponteiro que recebe o valor
 * @retval  true  - Campo existe e e numerico
 * @retval  false - Campo ausente ou de outro tipo
 */
static bool Fusion_GetNumber(const cJSON *parent, const char *key, double *value)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, key);

    if(cJSON_IsNumber(item) == 0)
    {
        return false;
    }

    *value = item->valuedouble;

    return true;
}

/*
 * @brief   Le um sub-objeto com os campos x, y e z.
 * @param   parent - Objeto que contem o sub-objeto
 * @param   key    - Nome do sub-objeto (ex: "accel")
 * @param   x      - Ponteiro que recebe x
 * @param   y      - Ponteiro que recebe y
 * @param   z      - Ponteiro que recebe z
 * @retval  true  - Os tres eixos foram lidos
 * @retval  false - Sub-objeto ou algum eixo ausente
 */
static bool Fusion_GetVector(const cJSON *parent,
                             const char *key,
                             float *x,
                             float *y,
                             float *z)
{
    const cJSON *object = cJSON_GetObjectItemCaseSensitive(parent, key);
    double vx;
    double vy;
    double vz;

    if((cJSON_IsObject(object) == 0)
       || (Fusion_GetNumber(object, "x", &vx) == false)
       || (Fusion_GetNumber(object, "y", &vy) == false)
       || (Fusion_GetNumber(object, "z", &vz) == false))
    {
        return false;
    }

    *x = (float)vx;
    *y = (float)vy;
    *z = (float)vz;

    return true;
}

/*
 * @brief   Trata uma mensagem de sensor/bmp/raw.
 * @param   payload    - Texto JSON recebido
 * @param   payloadLen - Tamanho do texto [bytes]
 * @retval  void
 */
static void Fusion_HandleBmp(const char *payload, size_t payloadLen)
{
    cJSON *root = cJSON_ParseWithLength(payload, payloadLen);
    double timestamp;
    double temperature;
    double pressure;
    double altitude;

    if(root == NULL)
    {
        return;
    }

    if((Fusion_GetNumber(root, "timestamp", &timestamp) == true)
       && (Fusion_GetNumber(root, "temperature", &temperature) == true)
       && (Fusion_GetNumber(root, "pressure", &pressure) == true)
       && (Fusion_GetNumber(root, "altitude", &altitude) == true))
    {
        pthread_mutex_lock(&fusion.mutex);

        fusion.bmp.data.timestamp = (int64_t)timestamp;
        fusion.bmp.data.temperature = (float)temperature;
        fusion.bmp.data.pressure = (float)pressure;
        fusion.bmp.data.altitude = (float)altitude;
        fusion.bmp.data.health = Fusion_EvaluateBmp(fusion.bmp.data.temperature,
                                                    fusion.bmp.data.pressure,
                                                    fusion.bmp.data.altitude);
        fusion.bmp.lastRxMs = Fusion_GetClockMs(CLOCK_MONOTONIC);
        fusion.bmp.valid = true;

        pthread_mutex_unlock(&fusion.mutex);
    }

    cJSON_Delete(root);
}

/*
 * @brief   Trata uma mensagem de fusion/bmp_mpu (usa apenas o objeto mpu).
 * @param   payload    - Texto JSON recebido
 * @param   payloadLen - Tamanho do texto [bytes]
 * @retval  void
 */
static void Fusion_HandleMpu(const char *payload, size_t payloadLen)
{
    cJSON *root = cJSON_ParseWithLength(payload, payloadLen);
    const cJSON *mpu;
    fusionMpu_t data;
    double timestamp;

    if(root == NULL)
    {
        return;
    }

    mpu = cJSON_GetObjectItemCaseSensitive(root, "mpu");

    if((cJSON_IsObject(mpu) != 0)
       && (Fusion_GetNumber(mpu, "timestamp", &timestamp) == true)
       && (Fusion_GetVector(mpu, "accel", &data.accelX, &data.accelY, &data.accelZ) == true)
       && (Fusion_GetVector(mpu, "gyro", &data.gyroX, &data.gyroY, &data.gyroZ) == true))
    {
        pthread_mutex_lock(&fusion.mutex);

        data.timestamp = (int64_t)timestamp;
        data.health = Fusion_EvaluateMpu(&data);
        fusion.mpu.data = data;
        fusion.mpu.lastRxMs = Fusion_GetClockMs(CLOCK_MONOTONIC);
        fusion.mpu.valid = true;

        pthread_mutex_unlock(&fusion.mutex);
    }

    cJSON_Delete(root);
}

/*
 * @brief   Trata uma mensagem de status/raspN (indicador de conexao).
 * @param   isBmpNode  - true = status/rasp1 (BMP), false = status/rasp2 (MPU)
 * @param   payload    - Texto JSON recebido
 * @param   payloadLen - Tamanho do texto [bytes]
 * @retval  void
 */
static void Fusion_HandleStatus(bool isBmpNode, const char *payload, size_t payloadLen)
{
    cJSON *root = cJSON_ParseWithLength(payload, payloadLen);
    const cJSON *status;
    bool online;

    if(root == NULL)
    {
        return;
    }

    status = cJSON_GetObjectItemCaseSensitive(root, "Status");

    if((cJSON_IsString(status) != 0) && (status->valuestring != NULL))
    {
        online = (strcmp(status->valuestring, dFUSION_STATUS_ONLINE) == 0);

        pthread_mutex_lock(&fusion.mutex);

        if(isBmpNode == true)
        {
            fusion.bmp.online = online;
        }
        else
        {
            fusion.mpu.online = online;
        }

        pthread_mutex_unlock(&fusion.mutex);
    }

    cJSON_Delete(root);
}

/*
 * @brief   Publica o status deste no (retido, QoS 1).
 * @param   status - "ONLINE" ou "OFFLINE"
 * @retval  void
 */
static void Fusion_PublishStatus(const char *status)
{
    cJSON *root = cJSON_CreateObject();
    char *text;

    if(root == NULL)
    {
        return;
    }

    cJSON_AddNumberToObject(root, "timestamp",
                            (double)Fusion_GetClockMs(CLOCK_REALTIME));
    cJSON_AddStringToObject(root, "Status", status);

    text = cJSON_PrintUnformatted(root);

    if(text != NULL)
    {
        mosquitto_publish(fusion.client, NULL, dFUSION_TOPIC_STATUS_RASP3,
                          (int)strlen(text), text, 1, true);
        cJSON_free(text);
    }

    cJSON_Delete(root);
}

/*
 * @brief   Callback de conexao: assina os topicos e publica ONLINE.
 *          Roda a cada (re)conexao, entao as assinaturas sao refeitas.
 * @param   client   - Cliente mosquitto
 * @param   userData - Nao utilizado
 * @param   result   - Codigo de retorno da conexao (0 = sucesso)
 * @retval  void
 */
static void Fusion_OnConnect(struct mosquitto *client, void *userData, int result)
{
    (void)userData;

    if(result != 0)
    {
        return;
    }

    mosquitto_subscribe(client, NULL, dFUSION_TOPIC_BMP_RAW, 0);
    mosquitto_subscribe(client, NULL, dFUSION_TOPIC_MPU_FUSION, 1);
    mosquitto_subscribe(client, NULL, dFUSION_TOPIC_STATUS_RASP1, 1);
    mosquitto_subscribe(client, NULL, dFUSION_TOPIC_STATUS_RASP2, 1);

    Fusion_PublishStatus(dFUSION_STATUS_ONLINE);
}

/*
 * @brief   Callback de desconexao: sem broker nao ha como saber o status
 *          dos outros nos, entao os dois indicadores voltam para offline.
 * @param   client   - Cliente mosquitto (nao utilizado)
 * @param   userData - Nao utilizado
 * @param   result   - Motivo da desconexao (nao utilizado)
 * @retval  void
 */
static void Fusion_OnDisconnect(struct mosquitto *client, void *userData, int result)
{
    (void)client;
    (void)userData;
    (void)result;

    pthread_mutex_lock(&fusion.mutex);
    fusion.bmp.online = false;
    fusion.mpu.online = false;
    pthread_mutex_unlock(&fusion.mutex);
}

/*
 * @brief   Callback de mensagem: direciona o payload para o tratador do topico.
 * @param   client   - Cliente mosquitto (nao utilizado)
 * @param   userData - Nao utilizado
 * @param   message  - Mensagem recebida
 * @retval  void
 */
static void Fusion_OnMessage(struct mosquitto *client,
                             void *userData,
                             const struct mosquitto_message *message)
{
    (void)client;
    (void)userData;

    if((message->payload == NULL) || (message->payloadlen <= 0))
    {
        return;
    }

    if(strcmp(message->topic, dFUSION_TOPIC_BMP_RAW) == 0)
    {
        Fusion_HandleBmp(message->payload, (size_t)message->payloadlen);
    }
    else if(strcmp(message->topic, dFUSION_TOPIC_MPU_FUSION) == 0)
    {
        Fusion_HandleMpu(message->payload, (size_t)message->payloadlen);
    }
    else if(strcmp(message->topic, dFUSION_TOPIC_STATUS_RASP1) == 0)
    {
        Fusion_HandleStatus(true, message->payload, (size_t)message->payloadlen);
    }
    else if(strcmp(message->topic, dFUSION_TOPIC_STATUS_RASP2) == 0)
    {
        Fusion_HandleStatus(false, message->payload, (size_t)message->payloadlen);
    }
}

bool Fusion_Init(const char *brokerHost, int brokerPort)
{
    if(fusion.initialized == true)
    {
        return true;
    }

    if(brokerHost == NULL)
    {
        return false;
    }

    memset(&fusion.bmp, 0, sizeof(fusion.bmp));
    memset(&fusion.mpu, 0, sizeof(fusion.mpu));

    mosquitto_lib_init();
    fusion.client = mosquitto_new(dFUSION_CLIENT_ID, true, NULL);

    if(fusion.client == NULL)
    {
        mosquitto_lib_cleanup();
        return false;
    }

    mosquitto_will_set(fusion.client, dFUSION_TOPIC_STATUS_RASP3,
                       (int)strlen(dFUSION_WILL_PAYLOAD), dFUSION_WILL_PAYLOAD,
                       1, true);
    mosquitto_connect_callback_set(fusion.client, Fusion_OnConnect);
    mosquitto_disconnect_callback_set(fusion.client, Fusion_OnDisconnect);
    mosquitto_message_callback_set(fusion.client, Fusion_OnMessage);
    mosquitto_reconnect_delay_set(fusion.client, 1, 10, false);

    if((mosquitto_connect_async(fusion.client, brokerHost, brokerPort,
                                dFUSION_KEEPALIVE_S) != MOSQ_ERR_SUCCESS)
       || (mosquitto_loop_start(fusion.client) != MOSQ_ERR_SUCCESS))
    {
        mosquitto_destroy(fusion.client);
        fusion.client = NULL;
        mosquitto_lib_cleanup();
        return false;
    }

    fusion.initialized = true;

    return true;
}

void Fusion_Deinit(void)
{
    if(fusion.initialized == false)
    {
        return;
    }

    Fusion_PublishStatus(dFUSION_STATUS_OFFLINE);
    mosquitto_disconnect(fusion.client);
    mosquitto_loop_stop(fusion.client, false);
    mosquitto_destroy(fusion.client);
    mosquitto_lib_cleanup();

    fusion.client = NULL;
    fusion.initialized = false;
}

bool Fusion_GetBmp(fusionBmp_t *data)
{
    uint64_t nowMs;

    if(data == NULL)
    {
        return false;
    }

    nowMs = Fusion_GetClockMs(CLOCK_MONOTONIC);

    pthread_mutex_lock(&fusion.mutex);

    *data = fusion.bmp.data;
    data->hasData = fusion.bmp.valid;
    data->connected = fusion.bmp.online;
    data->stale = Fusion_IsStale(fusion.bmp.valid, fusion.bmp.lastRxMs,
                                 nowMs, dFUSION_BMP_INTERVAL_MS);

    pthread_mutex_unlock(&fusion.mutex);

    return true;
}

bool Fusion_GetMpu(fusionMpu_t *data)
{
    uint64_t nowMs;

    if(data == NULL)
    {
        return false;
    }

    nowMs = Fusion_GetClockMs(CLOCK_MONOTONIC);

    pthread_mutex_lock(&fusion.mutex);

    *data = fusion.mpu.data;
    data->hasData = fusion.mpu.valid;
    data->connected = fusion.mpu.online;
    data->stale = Fusion_IsStale(fusion.mpu.valid, fusion.mpu.lastRxMs,
                                 nowMs, dFUSION_MPU_INTERVAL_MS);

    pthread_mutex_unlock(&fusion.mutex);

    return true;
}