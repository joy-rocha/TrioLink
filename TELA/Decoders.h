/**************************************
* @file Decoders.h
* @addtogroup Decoders
* @brief Processamento e decodificacao de dados de sensores BMP e MPU.
* @{
***************************************/

#ifndef _DECODERS_H_
#define _DECODERS_H_

/**************************************
* INCLUDES NECESSARIOS
***************************************/
#include <cjson/cJSON.h>

/**************************************
* CONFIGURACOES
***************************************/

/**************************************
* DEFINES PUBLICOS
***************************************/

/**************************************
* TIPOS DE DADOS PUBLICOS
***************************************/

/**************************************
* PROTOTIPOS PUBLICOS
***************************************/
/**************************************
/** @brief Processa e estrutura os dados brutos do sensor BMP280.
* @param rawJson: string contendo o JSON bruto retornado pelo sensor BMP.
* @retval Ponteiro para objeto cJSON com os dados processados ou NULL em caso de erro.
***************************************/
cJSON *DecodeBmp_ProcessData(const char *rawJson);

/**************************************
/** @brief Processa e estrutura os dados brutos do sensor MPU6050.
* @param rawJson: string contendo o JSON bruto retornado pelo sensor MPU.
* @retval Ponteiro para objeto cJSON com os dados processados ou NULL em caso de erro.
***************************************/
cJSON *DecodeMpu_ProcessData(const char *rawJson);

#endif /* _DECODERS_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */