/**************************************
* @file MpuPayloads.c
* @addtogroup MpuPayloads
* @brief Monta os tres payloads JSON publicados pelo No 2 (sensor/mpu/raw,
         fusion/bmp_mpu e status/rasp2), seguindo o schema combinado entre
         os tres nos (Schemas.md).
* @author Caio Oliveira
* @details
* \n <b>Ferramentas:</b> generico (Linux, Raspberry Pi 5).
* \n <b>Dependencias:</b>
* - AssertTypes.
* - BmpTracker (fornece bmpTrackerData_t, usado por MpuPayloads_BuildFusion).
* - Mpu6500Iio (fornece mpu6500Sample_t).
* - cJSON (link com -lcjson; header em <cjson/cJSON.h>).
* \n <b>Observacoes:</b>
* - Biblioteca sem estado: nao possui variaveis estaticas locais. As tres
*   funcoes publicas sao independentes entre si; a unica coisa que
*   compartilham e o helper local de arredondamento.
* - MpuPayloads_BuildFusion() reaproveita mpuSample->timestampMs tambem
*   como timestamp de GERACAO da mensagem de fusao (campo "timestamp" de
*   nivel superior do JSON), ja que a leitura do MPU acontece praticamente
*   no mesmo instante da publicacao.
* - Precisao decimal por campo: ver dMPU_PAYLOADS_*_DECIMALS abaixo. Nao e
*   um numero unico para todos os campos porque float guarda so ~7 digitos
*   decimais significativos NO TOTAL; pressure, por ter grandeza ~100000
*   (6 digitos inteiros), so comporta 1 casa decimal sem expor ruido de
*   representacao binaria do float no JSON. Accel/gyro, de grandeza
*   pequena, comportam ate 4 casas com folga.
* Changelog
* @version <b>1.0.0 - 17/10/2026</b> \n Caio Oliveira \n Primeira versao,
*          consolidando MpuPayload, FusionPayload e StatusPayload em um
*          unico arquivo.
* @copyright 
**************************************/

/**************************************
* INCLUDES
**************************************/
#include "MpuPayloads.h"
#include <cjson/cJSON.h>
#include <math.h>
#include <string.h>

/**************************************
* DEFINES LOCAIS
**************************************/
/// Nomes de campo usados nos tres JSONs, conforme o Schemas.md
#define dMPU_PAYLOADS_FIELD_TIMESTAMP      "timestamp"   // [string]
#define dMPU_PAYLOADS_FIELD_ACCEL          "accel"       // [string]
#define dMPU_PAYLOADS_FIELD_GYRO           "gyro"        // [string]
#define dMPU_PAYLOADS_FIELD_X              "x"           // [string]
#define dMPU_PAYLOADS_FIELD_Y              "y"           // [string]
#define dMPU_PAYLOADS_FIELD_Z              "z"           // [string]
#define dMPU_PAYLOADS_FIELD_BMP            "bmp"         // [string]
#define dMPU_PAYLOADS_FIELD_MPU            "mpu"         // [string]
#define dMPU_PAYLOADS_FIELD_TEMPERATURE    "temperature" // [string]
#define dMPU_PAYLOADS_FIELD_PRESSURE       "pressure"    // [string]
#define dMPU_PAYLOADS_FIELD_ALTITUDE       "altitude"    // [string]
#define dMPU_PAYLOADS_FIELD_STALE          "stale"       // [string]
#define dMPU_PAYLOADS_FIELD_STATUS         "Status"      // [string]
#define dMPU_PAYLOADS_VALUE_ONLINE         "ONLINE"      // [string]
#define dMPU_PAYLOADS_VALUE_OFFLINE        "OFFLINE"     // [string]

/// cJSON_PrintPreallocated: 0 = compacto (sem espacos/quebras de linha).
/// Payload MQTT deve ser compacto, para economizar bytes na rede.
#define dMPU_PAYLOADS_JSON_FORMAT_COMPACT  0

/// Casas decimais por campo (ver justificativa no cabecalho do arquivo)
#define dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS    4
#define dMPU_PAYLOADS_TEMPERATURE_DECIMALS   2
#define dMPU_PAYLOADS_PRESSURE_DECIMALS      1
#define dMPU_PAYLOADS_ALTITUDE_DECIMALS      2

/**************************************
* CONSTANTES
**************************************/
/* Nao ha constantes internas nesta biblioteca */

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
**************************************/
/* Esta biblioteca e sem estado: nao possui variaveis estaticas locais */

/**************************************
* PROTOTIPOS LOCAIS
**************************************/
static double Mpu_Payloads_Round(float value, u8 decimalPlaces);
static mpuPayloadsReturn_t Mpu_Payloads_Print(cJSON *root, char *jsonOut, u32 jsonOutLen, u32 *writtenLen);

/**************************************
* FUNCOES PUBLICAS
**************************************/
/**************************************
/** @brief Monta o payload JSON do topico sensor/mpu/raw.
* @param mpuSample: amostra a ser serializada (tipicamente obtida de
                     Mpu6500Iio_ReadSample() ou Mpu6500Iio_ReadAveragedSample()).
* @param jsonOut: buffer onde o JSON (string terminada em zero) sera
                  escrito. Ver dMPU_PAYLOADS_RAW_BUFFER_LEN.
* @param jsonOutLen: tamanho em bytes do buffer jsonOut.
* @param writtenLen: opcional (pode ser dNULL). Recebe a quantidade de
                      caracteres escritos, sem contar o terminador nulo.
* @retval eMPU_PAYLOADS_RETURN_OK se o JSON foi montado com sucesso.
          eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT se mpuSample ou jsonOut
          forem dNULL, ou jsonOutLen for zero.
          eMPU_PAYLOADS_RETURN_BUFFER_TOO_SMALL se jsonOutLen nao bastar.
          eMPU_PAYLOADS_RETURN_JSON_ERROR se a biblioteca cJSON falhar.
**************************************/
mpuPayloadsReturn_t MpuPayloads_BuildRaw(const mpu6500Sample_t *mpuSample, char *jsonOut, u32 jsonOutLen, u32 *writtenLen)
{
    mpuPayloadsReturn_t result = eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT;

    if((mpuSample == dNULL) || (jsonOut == dNULL) || (jsonOutLen == 0))
    {
        result = eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT;
    }
    else
    {
        cJSON *root = cJSON_CreateObject();

        if(root == dNULL)
        {
            result = eMPU_PAYLOADS_RETURN_JSON_ERROR;
        }
        else
        {
            cJSON *accel = cJSON_AddObjectToObject(root, dMPU_PAYLOADS_FIELD_ACCEL);
            cJSON *gyro  = cJSON_AddObjectToObject(root, dMPU_PAYLOADS_FIELD_GYRO);

            if((accel == dNULL) || (gyro == dNULL))
            {
                result = eMPU_PAYLOADS_RETURN_JSON_ERROR;
            }
            else
            {
                cJSON_AddNumberToObject(root, dMPU_PAYLOADS_FIELD_TIMESTAMP, (double)mpuSample->timestampMs);

                cJSON_AddNumberToObject(accel, dMPU_PAYLOADS_FIELD_X, Mpu_Payloads_Round(mpuSample->accelX, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));
                cJSON_AddNumberToObject(accel, dMPU_PAYLOADS_FIELD_Y, Mpu_Payloads_Round(mpuSample->accelY, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));
                cJSON_AddNumberToObject(accel, dMPU_PAYLOADS_FIELD_Z, Mpu_Payloads_Round(mpuSample->accelZ, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));

                cJSON_AddNumberToObject(gyro, dMPU_PAYLOADS_FIELD_X, Mpu_Payloads_Round(mpuSample->gyroX, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));
                cJSON_AddNumberToObject(gyro, dMPU_PAYLOADS_FIELD_Y, Mpu_Payloads_Round(mpuSample->gyroY, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));
                cJSON_AddNumberToObject(gyro, dMPU_PAYLOADS_FIELD_Z, Mpu_Payloads_Round(mpuSample->gyroZ, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));

                result = Mpu_Payloads_Print(root, jsonOut, jsonOutLen, writtenLen);
            }

            cJSON_Delete(root);
        }
    }
    return result;
}

/**************************************
/** @brief Monta o payload JSON do topico fusion/bmp_mpu, combinando a
           leitura propria do MPU6500 com o ultimo dado conhecido do
           BMP280 (No 1).
* @param mpuSample: leitura atual do MPU6500.
* @param bmpData: ultimo dado conhecido do BMP280, obtido via
                  BmpTracker_GetLatest() (ja com "stale" calculado).
* @param jsonOut: buffer onde o JSON sera escrito. Ver
                  dMPU_PAYLOADS_FUSION_BUFFER_LEN.
* @param jsonOutLen: tamanho em bytes do buffer jsonOut.
* @param writtenLen: opcional (pode ser dNULL).
* @retval eMPU_PAYLOADS_RETURN_OK se o JSON foi montado com sucesso.
          eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT se mpuSample, bmpData ou
          jsonOut forem dNULL, ou jsonOutLen for zero.
          eMPU_PAYLOADS_RETURN_BUFFER_TOO_SMALL se jsonOutLen nao bastar.
          eMPU_PAYLOADS_RETURN_JSON_ERROR se a biblioteca cJSON falhar.
* @details O objeto "mpu" dentro do JSON NAO tem campo "stale": o dado do
           MPU e sempre gerado ao vivo pelo proprio No 2 nesta mesma
           chamada, entao nunca fica obsoleto dentro deste topico.
**************************************/
mpuPayloadsReturn_t MpuPayloads_BuildFusion(const mpu6500Sample_t *mpuSample, const bmpTrackerData_t *bmpData, char *jsonOut, u32 jsonOutLen, u32 *writtenLen)
{
    mpuPayloadsReturn_t result = eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT;

    if((mpuSample == dNULL) || (bmpData == dNULL) || (jsonOut == dNULL) || (jsonOutLen == 0))
    {
        result = eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT;
    }
    else
    {
        cJSON *root = cJSON_CreateObject();

        if(root == dNULL)
        {
            result = eMPU_PAYLOADS_RETURN_JSON_ERROR;
        }
        else
        {
            cJSON *bmp   = cJSON_AddObjectToObject(root, dMPU_PAYLOADS_FIELD_BMP);
            cJSON *mpu   = cJSON_AddObjectToObject(root, dMPU_PAYLOADS_FIELD_MPU);
            cJSON *accel = (mpu != dNULL) ? cJSON_AddObjectToObject(mpu, dMPU_PAYLOADS_FIELD_ACCEL) : dNULL;
            cJSON *gyro  = (mpu != dNULL) ? cJSON_AddObjectToObject(mpu, dMPU_PAYLOADS_FIELD_GYRO)  : dNULL;

            if((bmp == dNULL) || (mpu == dNULL) || (accel == dNULL) || (gyro == dNULL))
            {
                result = eMPU_PAYLOADS_RETURN_JSON_ERROR;
            }
            else
            {
                cJSON_AddNumberToObject(root, dMPU_PAYLOADS_FIELD_TIMESTAMP, (double)mpuSample->timestampMs);

                cJSON_AddNumberToObject(bmp, dMPU_PAYLOADS_FIELD_TIMESTAMP, (double)bmpData->timestampMs);
                cJSON_AddNumberToObject(bmp, dMPU_PAYLOADS_FIELD_TEMPERATURE, Mpu_Payloads_Round(bmpData->temperature, dMPU_PAYLOADS_TEMPERATURE_DECIMALS));
                cJSON_AddNumberToObject(bmp, dMPU_PAYLOADS_FIELD_PRESSURE, Mpu_Payloads_Round(bmpData->pressure, dMPU_PAYLOADS_PRESSURE_DECIMALS));
                cJSON_AddNumberToObject(bmp, dMPU_PAYLOADS_FIELD_ALTITUDE, Mpu_Payloads_Round(bmpData->altitude, dMPU_PAYLOADS_ALTITUDE_DECIMALS));
                cJSON_AddBoolToObject(bmp, dMPU_PAYLOADS_FIELD_STALE, bmpData->stale);

                cJSON_AddNumberToObject(mpu, dMPU_PAYLOADS_FIELD_TIMESTAMP, (double)mpuSample->timestampMs);

                cJSON_AddNumberToObject(accel, dMPU_PAYLOADS_FIELD_X, Mpu_Payloads_Round(mpuSample->accelX, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));
                cJSON_AddNumberToObject(accel, dMPU_PAYLOADS_FIELD_Y, Mpu_Payloads_Round(mpuSample->accelY, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));
                cJSON_AddNumberToObject(accel, dMPU_PAYLOADS_FIELD_Z, Mpu_Payloads_Round(mpuSample->accelZ, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));

                cJSON_AddNumberToObject(gyro, dMPU_PAYLOADS_FIELD_X, Mpu_Payloads_Round(mpuSample->gyroX, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));
                cJSON_AddNumberToObject(gyro, dMPU_PAYLOADS_FIELD_Y, Mpu_Payloads_Round(mpuSample->gyroY, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));
                cJSON_AddNumberToObject(gyro, dMPU_PAYLOADS_FIELD_Z, Mpu_Payloads_Round(mpuSample->gyroZ, dMPU_PAYLOADS_ACCEL_GYRO_DECIMALS));

                result = Mpu_Payloads_Print(root, jsonOut, jsonOutLen, writtenLen);
            }

            cJSON_Delete(root);
        }
    }
    return result;
}

/**************************************
/** @brief Monta o payload JSON do topico status/rasp2.
* @param timestampMs: instante da leitura/geracao do status, em
                       milissegundos desde a epoca Unix (UTC).
* @param isOnline: true gera Status="ONLINE", false gera Status="OFFLINE".
* @param jsonOut: buffer onde o JSON sera escrito. Ver
                  dMPU_PAYLOADS_STATUS_BUFFER_LEN.
* @param jsonOutLen: tamanho em bytes do buffer jsonOut.
* @param writtenLen: opcional (pode ser dNULL).
* @retval eMPU_PAYLOADS_RETURN_OK se o JSON foi montado com sucesso.
          eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT se jsonOut for dNULL ou
          jsonOutLen for zero.
          eMPU_PAYLOADS_RETURN_BUFFER_TOO_SMALL se jsonOutLen nao bastar.
          eMPU_PAYLOADS_RETURN_JSON_ERROR se a biblioteca cJSON falhar.
* @details Este mesmo payload serve tanto para a publicacao normal de
           status quanto para o Last Will (LWT) configurado em
           mqttWrapperConfig_t.willPayload, chamando esta funcao com
           isOnline=false antes de conectar.
**************************************/
mpuPayloadsReturn_t MpuPayloads_BuildStatus(s64 timestampMs, bool isOnline, char *jsonOut, u32 jsonOutLen, u32 *writtenLen)
{
    mpuPayloadsReturn_t result = eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT;

    if((jsonOut == dNULL) || (jsonOutLen == 0))
    {
        result = eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT;
    }
    else
    {
        cJSON *root = cJSON_CreateObject();

        if(root == dNULL)
        {
            result = eMPU_PAYLOADS_RETURN_JSON_ERROR;
        }
        else
        {
            const char *statusValue = (isOnline == true) ? dMPU_PAYLOADS_VALUE_ONLINE : dMPU_PAYLOADS_VALUE_OFFLINE;

            cJSON_AddNumberToObject(root, dMPU_PAYLOADS_FIELD_TIMESTAMP, (double)timestampMs);
            cJSON_AddStringToObject(root, dMPU_PAYLOADS_FIELD_STATUS, statusValue);

            result = Mpu_Payloads_Print(root, jsonOut, jsonOutLen, writtenLen);

            cJSON_Delete(root);
        }
    }
    return result;
}

/**************************************
* FUNCOES LOCAIS
**************************************/
/**************************************
/** @brief Converte um float para double, arredondado a uma quantidade
           fixa de casas decimais.
* @param value: valor de entrada.
* @param decimalPlaces: quantidade de casas decimais a manter.
* @retval O valor arredondado, como double.
* @details Necessario porque float->double direto preserva o ruido de
           representacao binaria do float (ex.: 0.12f vira
           0.119999997317791... em double), o que o cJSON imprime por
           extenso. Arredondar antes evita um payload JSON poluido com
           casas decimais sem significado fisico algum.
**************************************/
static double Mpu_Payloads_Round(float value, u8 decimalPlaces)
{
    double factor = pow(10.0, (double)decimalPlaces);

    return round((double)value * factor) / factor;
}

/**************************************
/** @brief Imprime uma arvore cJSON ja pronta no buffer do chamador, em
           formato compacto, e preenche writtenLen se informado. Funcao
           compartilhada pelas tres funcoes publicas deste arquivo, para
           nao repetir a mesma logica de impressao/erro tres vezes.
* @param root: arvore cJSON ja montada (nao e liberada por esta funcao;
               quem chama continua responsavel por cJSON_Delete(root)).
* @param jsonOut: buffer de saida.
* @param jsonOutLen: tamanho em bytes do buffer jsonOut.
* @param writtenLen: opcional (pode ser dNULL).
* @retval eMPU_PAYLOADS_RETURN_OK ou eMPU_PAYLOADS_RETURN_BUFFER_TOO_SMALL.
**************************************/
static mpuPayloadsReturn_t Mpu_Payloads_Print(cJSON *root, char *jsonOut, u32 jsonOutLen, u32 *writtenLen)
{
    mpuPayloadsReturn_t result;

    if(cJSON_PrintPreallocated(root, jsonOut, (int)jsonOutLen, dMPU_PAYLOADS_JSON_FORMAT_COMPACT) == 1)
    {
        if(writtenLen != dNULL)
        {
            *writtenLen = (u32)strlen(jsonOut);
        }
        result = eMPU_PAYLOADS_RETURN_OK;
    }
    else
    {
        result = eMPU_PAYLOADS_RETURN_BUFFER_TOO_SMALL;
    }
    return result;
}
/** @} DOXYGEN GROUP TAG END OF FILE */
