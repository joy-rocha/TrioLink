/**************************************
* @file MpuPayloads.h
* @addtogroup MpuPayloads
* @{
**************************************/
#ifndef _MPU_PAYLOADS_H_
#define _MPU_PAYLOADS_H_

/**************************************
* INCLUDES NECESSARIOS
**************************************/
#include "AssertTypes.h"
#include "BmpTracker.h"
#include "Mpu6500Iio.h"

/**************************************
* CONFIGURACOES
**************************************/
/// Tamanho de buffer recomendado para sensor/mpu/raw
#define dMPU_PAYLOADS_RAW_BUFFER_LEN       256 // [bytes]
/// Tamanho de buffer recomendado para fusion/bmp_mpu
#define dMPU_PAYLOADS_FUSION_BUFFER_LEN    384 // [bytes]
/// Tamanho de buffer recomendado para status/rasp2
#define dMPU_PAYLOADS_STATUS_BUFFER_LEN    64  // [bytes]

/**************************************
* DEFINES PUBLICOS
**************************************/
/* Nao ha defines publicos nesta biblioteca */

/**************************************
* TIPOS DE DADOS PUBLICOS
**************************************/
/// Possiveis retornos das funcoes desta biblioteca
typedef enum
{
    eMPU_PAYLOADS_RETURN_OK,
    eMPU_PAYLOADS_RETURN_INVALID_ARGUMENT,
    eMPU_PAYLOADS_RETURN_BUFFER_TOO_SMALL,
    eMPU_PAYLOADS_RETURN_JSON_ERROR,

    eMPU_PAYLOADS_RETURN_END_ENUM
} mpuPayloadsReturn_t;

/**************************************
* PROTOTIPOS PUBLICOS
**************************************/
mpuPayloadsReturn_t MpuPayloads_BuildRaw(const mpu6500Sample_t *mpuSample, char *jsonOut, u32 jsonOutLen, u32 *writtenLen);
mpuPayloadsReturn_t MpuPayloads_BuildFusion(const mpu6500Sample_t *mpuSample, const bmpTrackerData_t *bmpData, char *jsonOut, u32 jsonOutLen, u32 *writtenLen);
mpuPayloadsReturn_t MpuPayloads_BuildStatus(s64 timestampMs, bool isOnline, char *jsonOut, u32 jsonOutLen, u32 *writtenLen);

#endif /* _MPU_PAYLOADS_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
