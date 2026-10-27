/**************************************
* @file IioSysfs.h
* @addtogroup IioSysfs
* @{
**************************************/
#ifndef _IIO_SYSFS_H_
#define _IIO_SYSFS_H_

/**************************************
* INCLUDES NECESSARIOS
**************************************/
#include "AssertTypes.h"

/**************************************
* CONFIGURACOES
**************************************/
/// Caminho base do sysfs onde o kernel expoe os dispositivos IIO
#define dIIO_SYSFS_BASE_PATH        "/sys/bus/iio/devices" // [string]

/**************************************
* DEFINES PUBLICOS
**************************************/
/// Tamanho maximo de um caminho de arquivo montado pela biblioteca
#define dIIO_SYSFS_MAX_PATH         512 // [bytes]

/**************************************
* TIPOS DE DADOS PUBLICOS
**************************************/
/// Possiveis retornos das funcoes desta biblioteca
typedef enum
{
    eIIO_SYSFS_RETURN_OK,
    eIIO_SYSFS_RETURN_INVALID_ARGUMENT,
    eIIO_SYSFS_RETURN_NOT_FOUND,
    eIIO_SYSFS_RETURN_READ_ERROR,

    eIIO_SYSFS_RETURN_END_ENUM
} iioSysfsReturn_t;

/**************************************
* PROTOTIPOS PUBLICOS
**************************************/
iioSysfsReturn_t IioSysfs_FindDevice(const char *nameMatch, char *devPathOut, u16 outLen);
iioSysfsReturn_t IioSysfs_ReadAttrDouble(const char *devPath, const char *attr, double *out);
iioSysfsReturn_t IioSysfs_ReadAttrLong(const char *devPath, const char *attr, s32 *out);

#endif /* _IIO_SYSFS_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
