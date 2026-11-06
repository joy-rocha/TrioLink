/**************************************
* @file DecodeMpu.c
* @addtogroup DecodeMpu
* @brief Modulo para processamento e decodificacao de dados do sensor MPU6050.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - cJSON
* - math
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 05/11/2026</b> \n Assert \n Primeira versao adaptada.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "Decoders.h"
#include <cjson/cJSON.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Fator de conversao de radianos para graus
#define dDECODE_MPU_DEG_CONV_FACTOR 180.0
/// Valor aproximado de PI
#define dDECODE_MPU_PI_VALUE        3.14159265
/// Graus de uma circunferencia completa
#define dDECODE_MPU_FULL_CIRCLE_DEG 360.0
/// Escala de arredondamento para uma casa decimal
#define dDECODE_MPU_ROUND_SCALE     10.0
/// Limite de aceleracao total para estado anormal
#define dDECODE_MPU_ACCEL_THRESHOLD 15.0
/// Potencia para calculo do quadrado
#define dDECODE_MPU_SQUARE_POWER    2

/**************************************
* CONSTANTES
***************************************/

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/

/**************************************
* PROTOTIPOS LOCAIS
***************************************/

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
/** @brief Processa e estrutura os dados brutos do sensor MPU6050.
* @param rawJson: string contendo o JSON bruto retornado pelo sensor MPU.
* @retval Ponteiro para objeto cJSON com os dados processados ou NULL em caso de erro.
***************************************/
cJSON *DecodeMpu_ProcessData(const char *rawJson)
{
    cJSON *jsonInput = cJSON_Parse(rawJson);
    if(jsonInput == NULL)
    {
        printf("Erro ao ler o JSON do MPU.\n");
        return NULL;
    }

    cJSON *accelObj = cJSON_GetObjectItemCaseSensitive(jsonInput, "accel");
    if(accelObj == NULL)
    {
        printf("Objeto 'accel' ausente no JSON do MPU.\n");
        cJSON_Delete(jsonInput);
        return NULL;
    }

    cJSON *accelX = cJSON_GetObjectItemCaseSensitive(accelObj, "x");
    cJSON *accelY = cJSON_GetObjectItemCaseSensitive(accelObj, "y");
    cJSON *accelZ = cJSON_GetObjectItemCaseSensitive(accelObj, "z");

    if((accelX == NULL) || (accelY == NULL) || (accelZ == NULL))
    {
        printf("Faltam coordenadas do acelerometro no JSON do MPU.\n");
        cJSON_Delete(jsonInput);
        return NULL;
    }

    double directionDegrees = atan2(accelY->valuedouble, accelX->valuedouble) * (dDECODE_MPU_DEG_CONV_FACTOR / dDECODE_MPU_PI_VALUE);
    if(directionDegrees < 0.0)
    {
        directionDegrees += dDECODE_MPU_FULL_CIRCLE_DEG;
    }

    double totalAcceleration = sqrt(pow(accelX->valuedouble, dDECODE_MPU_SQUARE_POWER) +
                                    pow(accelY->valuedouble, dDECODE_MPU_SQUARE_POWER) +
                                    pow(accelZ->valuedouble, dDECODE_MPU_SQUARE_POWER));

    cJSON *jsonMpu = cJSON_CreateObject();
    if(jsonMpu == NULL)
    {
        cJSON_Delete(jsonInput);
        return NULL;
    }

    cJSON_AddStringToObject(jsonMpu, "status", "online");
    cJSON_AddNumberToObject(jsonMpu, "direcao", round(directionDegrees * dDECODE_MPU_ROUND_SCALE) / dDECODE_MPU_ROUND_SCALE);
    cJSON_AddNumberToObject(jsonMpu, "velocidade", round(totalAcceleration * dDECODE_MPU_ROUND_SCALE) / dDECODE_MPU_ROUND_SCALE);

    if(totalAcceleration > dDECODE_MPU_ACCEL_THRESHOLD)
    {
        cJSON_AddStringToObject(jsonMpu, "estado", "anormal");
    }
    else
    {
        cJSON_AddStringToObject(jsonMpu, "estado", "normal");
    }

    cJSON_Delete(jsonInput);

    return jsonMpu;
}

/**************************************
* FUNCOES LOCAIS
***************************************/
/** @} DOXYGEN GROUP TAG END OF FILE */