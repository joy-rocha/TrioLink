/**************************************
* @file DecodeMpu.c
* @addtogroup Decoders
* @brief Decodificador de dados do sensor MPU6050.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - cJSON
* - math.h
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 05/11/2026</b> \n Assert \n Primeira versao adaptada.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "Decoders.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Valor da constante PI
#define dDECODE_MPU_PI           3.14159265
/// Fator de conversao de radianos para graus
#define dDECODE_MPU_RAD_TO_DEG   (180.0 / dDECODE_MPU_PI)
/// Graus de uma volta completa
#define dDECODE_MPU_FULL_CIRCLE  360.0
/// Limite de aceleracao total para estado anormal
#define dDECODE_MPU_ACCEL_LIMIT  15.0

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
* @brief Processa e decodifica o JSON de dados do sensor MPU6050.
* @param jsonRawMpu: string JSON bruta do sensor.
* @retval Ponteiro para objeto cJSON com os dados refinados, ou NULL em caso de erro.
***************************************/
cJSON *DecodeMpu_ProcessData(const char *jsonRawMpu)
{
    cJSON *jsonIn = cJSON_Parse(jsonRawMpu);
    if(jsonIn == NULL)
    {
        printf("Erro ao ler o JSON do MPU.\n");
        return NULL;
    }

    cJSON *accelObject = cJSON_GetObjectItemCaseSensitive(jsonIn, "accel");
    if(accelObject == NULL)
    {
        printf("Objeto 'accel' ausente no JSON do MPU.\n");
        cJSON_Delete(jsonIn);
        return NULL;
    }

    cJSON *accelX = cJSON_GetObjectItemCaseSensitive(accelObject, "x");
    cJSON *accelY = cJSON_GetObjectItemCaseSensitive(accelObject, "y");
    cJSON *accelZ = cJSON_GetObjectItemCaseSensitive(accelObject, "z");

    if((accelX == NULL) || (accelY == NULL) || (accelZ == NULL))
    {
        printf("Coordenadas incompletas no JSON do MPU.\n");
        cJSON_Delete(jsonIn);
        return NULL;
    }

    double directionDegrees = atan2(accelY->valuedouble, accelX->valuedouble) * dDECODE_MPU_RAD_TO_DEG;
    if(directionDegrees < 0.0)
    {
        directionDegrees += dDECODE_MPU_FULL_CIRCLE;
    }

    double totalAcceleration = sqrt(pow(accelX->valuedouble, 2) + pow(accelY->valuedouble, 2) + pow(accelZ->valuedouble, 2));

    cJSON *jsonMpu = cJSON_CreateObject();
    cJSON_AddStringToObject(jsonMpu, "status", "online");
    cJSON_AddNumberToObject(jsonMpu, "direcao", round(directionDegrees * 10.0) / 10.0);
    cJSON_AddNumberToObject(jsonMpu, "velocidade", round(totalAcceleration * 10.0) / 10.0);

    if(totalAcceleration > dDECODE_MPU_ACCEL_LIMIT)
    {
        cJSON_AddStringToObject(jsonMpu, "estado", "anormal");
    }
    else
    {
        cJSON_AddStringToObject(jsonMpu, "estado", "normal");
    }

    cJSON_Delete(jsonIn);
    return jsonMpu;
}

/**************************************
* FUNCOES LOCAIS
***************************************/
/** @} DOXYGEN GROUP TAG END OF FILE */