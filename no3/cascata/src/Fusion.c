/*
 * @file    Fusion.c
 * @brief   Implementacao do recebimento MQTT (fusion/bmp_mpu e status/#),
 *          parse cJSON, staleness, filtragem de status retido e analise
 *          dos dados dos sensores.
 */

#include "Fusion.h"

#include "MqttWrapper.h"

#include <cjson/cJSON.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

/// Topicos assinados
#define dFUSION_TOPIC_BMP_MPU           "fusion/bmp_mpu"
#define dFUSION_TOPIC_STATUS_ALL        "status/#"
#define dFUSION_TOPIC_STATUS_RASP1      "status/rasp1"   // BMP
#define dFUSION_TOPIC_STATUS_RASP2      "status/rasp2"   // MPU

/// Valores do campo Status
#define dFUSION_STATUS_ONLINE           "ONLINE"
#define dFUSION_STATUS_OFFLINE          "OFFLINE"
#define dFUSION_STATUS_TEXT_LEN         32

/// OBS: a conexao MQTT (TLS, usuario/senha, Last Will e status/rasp3) e feita
/// pelo Main via MqttWrapper; este modulo apenas assina e interpreta os dados.

/// Intervalo de publicacao esperado de fusion/bmp_mpu [ms]
#define dFUSION_BMP_INTERVAL_MS         1000
#define dFUSION_MPU_INTERVAL_MS         1000

/// Dado e obsoleto apos este multiplo do intervalo esperado
#define dFUSION_STALENESS_FACTOR        3

/// Janela inicial apos Subscribe [ms] em que o broker descarrega mensagens
/// retidas antigas. Mensagens "ONLINE" recebidas nessa janela ficam pendentes
/// ate confirmacao por trafego ao vivo, evitando iniciar em ONLINE se o no
/// estiver desligado.
#define dFUSION_RETAINED_WINDOW_MS      300U

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
    pthread_mutex_t mutex;
    uint64_t subscribeMs;

    struct
    {
        bool valid;
        bool online;
        bool pendingRetainedOnline;
        bool remoteStale;
        uint64_t lastRxMs;
        fusionBmp_t data;
    } bmp;

    struct
    {
        bool valid;
        bool online;
        bool pendingRetainedOnline;
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
 * @brief   Indica se a mensagem chegou dentro da janela inicial de descarga
 *          de mensagens retidas logo apos Fusion_Subscribe().
 *          Deve ser chamada com fusion.mutex travado.
 * @param   nowMs - Instante atual [ms, monotonico]
 * @retval  true  - Dentro da janela inicial (possivel mensagem retida)
 * @retval  false - Trafego ao vivo apos a janela inicial
 */
static bool Fusion_IsInInitialRetainedWindow(uint64_t nowMs)
{
    if(fusion.subscribeMs == 0U)
    {
        return true;
    }

    return ((nowMs - fusion.subscribeMs) <= (uint64_t)dFUSION_RETAINED_WINDOW_MS);
}

/*
 * @brief   Verifica se o dado esta obsoleto por timeout local.
 * @param   valid      - true se ja houve alguma leitura valida
 * @param   lastRxMs   - Instante da ultima mensagem ao vivo [ms, monotonico]
 * @param   nowMs      - Instante atual [ms, monotonico]
 * @param   intervalMs - Intervalo de publicacao esperado [ms]
 * @retval  true  - Dado obsoleto (ou inexistente / apenas retido)
 * @retval  false - Dado dentro do prazo
 */
static bool Fusion_IsStale(bool valid,
                           uint64_t lastRxMs,
                           uint64_t nowMs,
                           uint32_t intervalMs)
{
    if((valid == false) || (lastRxMs == 0U))
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
 * @brief   Le um campo booleano de um objeto cJSON (opcional com valor padrao).
 * @param   parent       - Objeto que contem o campo
 * @param   key          - Nome do campo
 * @param   defaultValue - Valor retornado caso ausente ou nao booleano
 * @retval  Valor booleano lido ou defaultValue
 */
static bool Fusion_GetBoolOrDefault(const cJSON *parent, const char *key, bool defaultValue)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(parent, key);

    if(cJSON_IsBool(item) == 0)
    {
        return defaultValue;
    }

    return (cJSON_IsTrue(item) != 0);
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
 * @brief   Extrai e atualiza o sub-objeto "bmp" de fusion/bmp_mpu.
 * @param   bmpObj - Sub-objeto JSON "bmp"
 * @param   nowMs  - Instante atual [ms, monotonico]
 * @retval  true  - Sub-objeto valido processado
 * @retval  false - Sub-objeto ausente ou invalido
 */
static bool Fusion_ParseBmpObject(const cJSON *bmpObj, uint64_t nowMs)
{
    double timestamp;
    double temperature;
    double pressure;
    double altitude = 0.0;
    bool remoteStale;
    bool isInitialBurst;

    if((cJSON_IsObject(bmpObj) == 0)
       || (Fusion_GetNumber(bmpObj, "timestamp", &timestamp) == false)
       || (Fusion_GetNumber(bmpObj, "temperature", &temperature) == false)
       || (Fusion_GetNumber(bmpObj, "pressure", &pressure) == false))
    {
        return false;
    }

    (void)Fusion_GetNumber(bmpObj, "altitude", &altitude);
    remoteStale = Fusion_GetBoolOrDefault(bmpObj, "stale", false);

    pthread_mutex_lock(&fusion.mutex);

    isInitialBurst = Fusion_IsInInitialRetainedWindow(nowMs);

    fusion.bmp.data.timestamp = (int64_t)timestamp;
    fusion.bmp.data.temperature = (float)temperature;
    fusion.bmp.data.pressure = (float)pressure;
    fusion.bmp.data.altitude = (float)altitude;
    fusion.bmp.data.health = Fusion_EvaluateBmp(fusion.bmp.data.temperature,
                                                fusion.bmp.data.pressure,
                                                fusion.bmp.data.altitude);
    fusion.bmp.remoteStale = remoteStale;
    fusion.bmp.valid = true;

    if(isInitialBurst == false)
    {
        fusion.bmp.lastRxMs = nowMs;

        if((fusion.bmp.pendingRetainedOnline == true) && (remoteStale == false))
        {
            fusion.bmp.online = true;
            fusion.bmp.pendingRetainedOnline = false;
        }
    }

    pthread_mutex_unlock(&fusion.mutex);

    return true;
}

/*
 * @brief   Extrai e atualiza o sub-objeto "mpu" de fusion/bmp_mpu.
 * @param   mpuObj - Sub-objeto JSON "mpu"
 * @param   nowMs  - Instante atual [ms, monotonico]
 * @retval  true  - Sub-objeto valido processado
 * @retval  false - Sub-objeto ausente ou invalido
 */
static bool Fusion_ParseMpuObject(const cJSON *mpuObj, uint64_t nowMs)
{
    fusionMpu_t data;
    double timestamp;
    bool isInitialBurst;

    if(cJSON_IsObject(mpuObj) == 0)
    {
        return false;
    }

    memset(&data, 0, sizeof(data));

    if((Fusion_GetNumber(mpuObj, "timestamp", &timestamp) == false)
       || (Fusion_GetVector(mpuObj, "accel", &data.accelX, &data.accelY, &data.accelZ) == false)
       || (Fusion_GetVector(mpuObj, "gyro", &data.gyroX, &data.gyroY, &data.gyroZ) == false))
    {
        return false;
    }

    data.timestamp = (int64_t)timestamp;
    data.health = Fusion_EvaluateMpu(&data);

    pthread_mutex_lock(&fusion.mutex);

    isInitialBurst = Fusion_IsInInitialRetainedWindow(nowMs);

    fusion.mpu.data = data;
    fusion.mpu.valid = true;

    if(isInitialBurst == false)
    {
        fusion.mpu.lastRxMs = nowMs;

        if(fusion.mpu.pendingRetainedOnline == true)
        {
            fusion.mpu.online = true;
            fusion.mpu.pendingRetainedOnline = false;
        }
    }

    pthread_mutex_unlock(&fusion.mutex);

    return true;
}

/*
 * @brief   Trata uma mensagem consolidada de fusion/bmp_mpu:
 *          { "timestamp": ..., "bmp": {...}, "mpu": {...} }
 * @param   payload    - Texto JSON recebido
 * @param   payloadLen - Tamanho do texto [bytes]
 * @retval  void
 */
static void Fusion_HandleBmpMpu(const char *payload, size_t payloadLen)
{
    cJSON *root = cJSON_ParseWithLength(payload, payloadLen);
    const cJSON *bmpObj;
    const cJSON *mpuObj;
    uint64_t nowMs;
    bool bmpOk;
    bool mpuOk;

    if(root == NULL)
    {
        (void)fprintf(stderr, "[fusion] AVISO: JSON invalido em %s\n", dFUSION_TOPIC_BMP_MPU);
        return;
    }

    nowMs = Fusion_GetClockMs(CLOCK_MONOTONIC);
    bmpObj = cJSON_GetObjectItemCaseSensitive(root, "bmp");
    mpuObj = cJSON_GetObjectItemCaseSensitive(root, "mpu");

    bmpOk = Fusion_ParseBmpObject(bmpObj, nowMs);
    mpuOk = Fusion_ParseMpuObject(mpuObj, nowMs);

    if((bmpOk == true) || (mpuOk == true))
    {
        (void)printf("[fusion] %s recebido: %.*s\n",
                     dFUSION_TOPIC_BMP_MPU, (int)payloadLen, payload);
    }
    else
    {
        (void)fprintf(stderr, "[fusion] AVISO: campos bmp/mpu ausentes em %s\n",
                      dFUSION_TOPIC_BMP_MPU);
    }

    cJSON_Delete(root);
}

/*
 * @brief   Interpreta o payload de status: aceita JSON {"Status":"ONLINE"}
 *          (chave "Status"/"status"), string JSON ("ONLINE") ou texto puro
 *          ONLINE/OFFLINE, sem diferenciar maiusculas de minusculas.
 * @param   payload    - Conteudo recebido
 * @param   payloadLen - Tamanho do conteudo [bytes]
 * @param   online     - Recebe true (ONLINE) ou false (OFFLINE)
 * @retval  true  - Payload reconhecido
 * @retval  false - Payload sem status valido (deve ser ignorado)
 */
static bool Fusion_ParseStatus(const char *payload, size_t payloadLen, bool *online)
{
    cJSON *root = cJSON_ParseWithLength(payload, payloadLen);
    const cJSON *status = NULL;
    const char *text = NULL;
    char plain[dFUSION_STATUS_TEXT_LEN];
    bool valid = false;

    if(root != NULL)
    {
        if(cJSON_IsObject(root) != 0)
        {
            status = cJSON_GetObjectItemCaseSensitive(root, "Status");

            if(status == NULL)
            {
                status = cJSON_GetObjectItemCaseSensitive(root, "status");
            }
        }
        else
        {
            status = root;
        }

        if((cJSON_IsString(status) != 0) && (status->valuestring != NULL))
        {
            text = status->valuestring;
        }
    }
    else if(payloadLen < sizeof(plain))
    {
        // Texto puro: copia sem espacos/quebras nas pontas
        size_t start = 0;
        size_t length = payloadLen;

        while((start < length) && (payload[start] <= ' '))
        {
            start++;
        }

        while((length > start) && (payload[length - 1U] <= ' '))
        {
            length--;
        }

        memcpy(plain, &payload[start], length - start);
        plain[length - start] = '\0';
        text = plain;
    }

    if(text != NULL)
    {
        if(strcasecmp(text, dFUSION_STATUS_ONLINE) == 0)
        {
            *online = true;
            valid = true;
        }
        else if(strcasecmp(text, dFUSION_STATUS_OFFLINE) == 0)
        {
            *online = false;
            valid = true;
        }
    }

    cJSON_Delete(root);   // aceita NULL

    return valid;
}

/*
 * @brief   Trata uma mensagem de status/raspN (indicador de conexao).
 *          Se "ONLINE" chegar na janela inicial de descarga de retidos logo
 *          apos Subscribe, aguarda sinal vivo antes de marcar ONLINE, evitando
 *          comecar em ONLINE quando o no esta desligado.
 * @param   isBmpNode  - true = status/rasp1 (BMP), false = status/rasp2 (MPU)
 * @param   payload    - Conteudo recebido
 * @param   payloadLen - Tamanho do conteudo [bytes]
 * @retval  void
 */
static void Fusion_HandleStatus(bool isBmpNode, const char *payload, size_t payloadLen)
{
    bool online = false;
    uint64_t nowMs;
    bool isInitialBurst;
    bool effectiveOnline;

    if(Fusion_ParseStatus(payload, payloadLen, &online) == false)
    {
        return;
    }

    nowMs = Fusion_GetClockMs(CLOCK_MONOTONIC);

    pthread_mutex_lock(&fusion.mutex);

    isInitialBurst = Fusion_IsInInitialRetainedWindow(nowMs);

    if(isBmpNode == true)
    {
        if(online == false)
        {
            fusion.bmp.online = false;
            fusion.bmp.pendingRetainedOnline = false;
        }
        else if(isInitialBurst == false)
        {
            fusion.bmp.online = true;
            fusion.bmp.pendingRetainedOnline = false;
        }
        else
        {
            fusion.bmp.pendingRetainedOnline = true;
        }

        effectiveOnline = fusion.bmp.online;
    }
    else
    {
        if(online == false)
        {
            fusion.mpu.online = false;
            fusion.mpu.pendingRetainedOnline = false;
        }
        else if(isInitialBurst == false)
        {
            fusion.mpu.online = true;
            fusion.mpu.pendingRetainedOnline = false;
        }
        else
        {
            fusion.mpu.pendingRetainedOnline = true;
        }

        effectiveOnline = fusion.mpu.online;
    }

    pthread_mutex_unlock(&fusion.mutex);

    (void)printf("[fusion] %s -> %s%s\n",
                 (isBmpNode == true) ? dFUSION_TOPIC_STATUS_RASP1 : dFUSION_TOPIC_STATUS_RASP2,
                 (effectiveOnline == true) ? dFUSION_STATUS_ONLINE : dFUSION_STATUS_OFFLINE,
                 ((online == true) && (isInitialBurst == true)) ? " (aguardando sinal vivo)" : "");
}

void Fusion_Init(void)
{
    pthread_mutex_lock(&fusion.mutex);
    fusion.subscribeMs = 0U;
    memset(&fusion.bmp, 0, sizeof(fusion.bmp));
    memset(&fusion.mpu, 0, sizeof(fusion.mpu));
    pthread_mutex_unlock(&fusion.mutex);
}

bool Fusion_Subscribe(void)
{
    bool ok = true;

    pthread_mutex_lock(&fusion.mutex);
    fusion.subscribeMs = Fusion_GetClockMs(CLOCK_MONOTONIC);
    fusion.bmp.online = false;
    fusion.bmp.pendingRetainedOnline = false;
    fusion.bmp.lastRxMs = 0U;
    fusion.mpu.online = false;
    fusion.mpu.pendingRetainedOnline = false;
    fusion.mpu.lastRxMs = 0U;
    pthread_mutex_unlock(&fusion.mutex);

    ok &= (Mqtt_Wrapper_Subscribe(dFUSION_TOPIC_BMP_MPU, eMQTT_WRAPPER_QOS_1)
           == eMQTT_WRAPPER_RETURN_OK);
    ok &= (Mqtt_Wrapper_Subscribe(dFUSION_TOPIC_STATUS_ALL, eMQTT_WRAPPER_QOS_1)
           == eMQTT_WRAPPER_RETURN_OK);

    return ok;
}

void Fusion_OnMessage(const char *topic, const char *payload, uint32_t payloadLen)
{
    if((topic == NULL) || (payload == NULL) || (payloadLen == 0U))
    {
        return;
    }

    if(strcmp(topic, dFUSION_TOPIC_BMP_MPU) == 0)
    {
        Fusion_HandleBmpMpu(payload, (size_t)payloadLen);
    }
    else if(strcmp(topic, dFUSION_TOPIC_STATUS_RASP1) == 0)
    {
        Fusion_HandleStatus(true, payload, (size_t)payloadLen);
    }
    else if(strcmp(topic, dFUSION_TOPIC_STATUS_RASP2) == 0)
    {
        Fusion_HandleStatus(false, payload, (size_t)payloadLen);
    }
}

void Fusion_OnBrokerDisconnected(void)
{
    pthread_mutex_lock(&fusion.mutex);
    fusion.bmp.online = false;
    fusion.bmp.pendingRetainedOnline = false;
    fusion.mpu.online = false;
    fusion.mpu.pendingRetainedOnline = false;
    pthread_mutex_unlock(&fusion.mutex);
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
    data->stale = (fusion.bmp.remoteStale == true)
                  || Fusion_IsStale(fusion.bmp.valid, fusion.bmp.lastRxMs,
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
