/**************************************
* @file DecodeBmp.c
* @addtogroup DecodeBmp
* @brief Modulo para processamento e decodificacao de dados do sensor BMP.
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
#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Divisor para conversao de Pascal para Hectopascal
#define dDECODE_BMP_PA_TO_HPA_DIVISOR       100.0
/// Limite de temperatura para estado anormal
#define dDECODE_BMP_TEMP_ANORMAL_THRESHOLD 57.0

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
/** @brief Processa e estrutura os dados brutos do sensor BMP280.
* @param rawJson: string contendo o JSON bruto retornado pelo sensor BMP.
* @retval Ponteiro para objeto cJSON com os dados processados ou NULL em caso de erro.
***************************************/
cJSON *DecodeBmp_ProcessData(const char *rawJson)
{
    cJSON *jsonInput = cJSON_Parse(rawJson);
    if(jsonInput == NULL)
    {
        printf("Erro ao ler o JSON do BMP.\n");
        return NULL;
    }

    cJSON *tempItem = cJSON_GetObjectItemCaseSensitive(jsonInput, "temperature");
    cJSON *pressItem = cJSON_GetObjectItemCaseSensitive(jsonInput, "pressure");
    cJSON *altItem = cJSON_GetObjectItemCaseSensitive(jsonInput, "altitude");

    if((tempItem == NULL) || (pressItem == NULL) || (altItem == NULL))
    {
        printf("Faltam dados essenciais no JSON do BMP.\n");
        cJSON_Delete(jsonInput);
        return NULL;
    }

    cJSON *jsonBmp = cJSON_CreateObject();
    if(jsonBmp == NULL)
    {
        cJSON_Delete(jsonInput);
        return NULL;
    }

    cJSON_AddStringToObject(jsonBmp, "status", "online");
    cJSON_AddNumberToObject(jsonBmp, "temperatura", tempItem->valuedouble);
    cJSON_AddNumberToObject(jsonBmp, "pressao", pressItem->valuedouble / dDECODE_BMP_PA_TO_HPA_DIVISOR);
    cJSON_AddNumberToObject(jsonBmp, "altitude", altItem->valuedouble);

    if(tempItem->valuedouble >= dDECODE_BMP_TEMP_ANORMAL_THRESHOLD)
    {
        cJSON_AddStringToObject(jsonBmp, "estado", "anormal");
    }
    else
    {
        cJSON_AddStringToObject(jsonBmp, "estado", "normal");
    }

    cJSON_Delete(jsonInput);

    return jsonBmp;
}

/**************************************
* FUNCOES LOCAIS
***************************************/
/** @} DOXYGEN GROUP TAG END OF FILE */