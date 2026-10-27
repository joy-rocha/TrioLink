/**************************************
* @file BmpTracker.h
* @addtogroup BmpTracker
* @{
**************************************/
#ifndef _BMP_TRACKER_H_
#define _BMP_TRACKER_H_

/**************************************
* INCLUDES NECESSARIOS
**************************************/
#include "AssertTypes.h"

/**************************************
* CONFIGURACOES
**************************************/
/// Tempo maximo (ms) sem receber sensor/bmp/raw antes do dado ser
/// considerado obsoleto (ver Schemas.md, secao "Regra de staleness").
#define dBMP_TRACKER_STALE_THRESHOLD_MS    3000 // [ms]

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
    eBMP_TRACKER_RETURN_OK,
    eBMP_TRACKER_RETURN_INVALID_ARGUMENT,
    eBMP_TRACKER_RETURN_NOT_INITIALIZED,
    eBMP_TRACKER_RETURN_PARSE_ERROR,

    eBMP_TRACKER_RETURN_END_ENUM
} bmpTrackerReturn_t;

/// Ultimo dado conhecido de sensor/bmp/raw, ja com a flag de obsolescencia calculada. 
/// Enquanto nenhuma mensagem valida tiver chegado, os campos
/// numericos ficam zerados e stale permanece true.
typedef struct
{
    /// Momento da aquisicao ORIGINAL da leitura no No 1 (nao avanca
    /// enquanto stale == true; repassado sem alteracao pelo No 2)
    s64 timestampMs;
    /// Temperatura. Unidade: C
    float temperature;
    /// Pressao. Unidade: Pa
    float pressure;
    /// Altitude. Unidade: m
    float altitude;
    /// true se sensor/bmp/raw nao chegou dentro do threshold de staleness
    bool stale;
} bmpTrackerData_t;

/**************************************
* PROTOTIPOS PUBLICOS
**************************************/
bmpTrackerReturn_t BmpTracker_Init(void);
bmpTrackerReturn_t BmpTracker_OnMessageReceived(const char *payload, u32 payloadLen, s64 nowMs);
bmpTrackerReturn_t BmpTracker_GetLatest(s64 nowMs, bmpTrackerData_t *dataOut);

#endif /* _BMP_TRACKER_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
