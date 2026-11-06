/**************************************
* @file Decoders.h
* @addtogroup Decoders
* @brief Modulo de decodificacao de dados JSON dos sensores BMP280 e MPU6050.
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
* @brief Processa e decodifica o JSON de dados do sensor BMP280.
* @param jsonRawBmp: string JSON bruta do sensor.
* @retval Ponteiro para objeto cJSON com os dados refinados, ou NULL em caso de erro.
***************************************/
cJSON *DecodeBmp_ProcessData(const char *jsonRawBmp);

/**************************************
* @brief Processa e decodifica o JSON de dados do sensor MPU6050.
* @param jsonRawMpu: string JSON bruta do sensor.
* @retval Ponteiro para objeto cJSON com os dados refinados, ou NULL em caso de erro.
***************************************/
cJSON *DecodeMpu_ProcessData(const char *jsonRawMpu);

#endif /* _DECODERS_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */