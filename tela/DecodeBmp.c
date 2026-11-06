/**************************************
* @file DecodeBmp.c
* @addtogroup Decoders
* @brief Decodificador de dados do sensor BMP280.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - cJSON
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 05/11/2026</b> \n Assert \n Primeira versao adaptada.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "Decoders.h"
#include <stdio.h>
#include <stdlib.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Fator de conversao de Pa para hPa
#define dDECODE_BMP_PA_TO_HPA    100.0
/// Limite de temperatura para estado anormal
#define dDECODE_BMP_TEMP_LIMIT   57.0

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
* @brief Processa e decodifica o JSON de dados do sensor BMP280.
* @param jsonRawBmp: string JSON bruta do sensor.
* @retval Ponteiro para objeto cJSON com os dados refinados, ou NULL em caso de erro.
***************************************/
cJSON *DecodeBmp_ProcessData(const char *jsonRawBmp)
{
    cJSON *jsonIn = cJSON_Parse(jsonRawBmp);
    if(jsonIn == NULL)
    {
        printf("Erro ao ler o JSON do BMP.\n");
        return NULL;
    }

    cJSON *tempItem = cJSON_GetObjectItemCaseSensitive(jsonIn, "temperature");
    cJSON *pressItem = cJSON_GetObjectItemCaseSensitive(jsonIn, "pressure");
    cJSON *altItem = cJSON_GetObjectItemCaseSensitive(jsonIn, "altitude");

    if((tempItem == NULL) || (pressItem == NULL) || (altItem == NULL))
    {
        printf("Faltam dados essenciais no JSON do BMP.\n");
        cJSON_Delete(jsonIn);
        return NULL;
    }

    cJSON *jsonBmp = cJSON_CreateObject();
    cJSON_AddStringToObject(jsonBmp, "status", "online");
    cJSON_AddNumberToObject(jsonBmp, "temperatura", tempItem->valuedouble);
    cJSON_AddNumberToObject(jsonBmp, "pressao", pressItem->valuedouble / dDECODE_BMP_PA_TO_HPA);
    cJSON_AddNumberToObject(jsonBmp, "altitude", altItem->valuedouble);

    if(tempItem->valuedouble >= dDECODE_BMP_TEMP_LIMIT)
    {
        cJSON_AddStringToObject(jsonBmp, "estado", "anormal");
    }
    else
    {
        cJSON_AddStringToObject(jsonBmp, "estado", "normal");
    }

    cJSON_Delete(jsonIn);
    return jsonBmp;
}

/**************************************
* FUNCOES LOCAIS
***************************************/
/** @} DOXYGEN GROUP TAG END OF FILE */