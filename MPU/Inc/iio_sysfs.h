#ifndef IIO_SYSFS_H
#define IIO_SYSFS_H

#include <stddef.h>

#define IIO_SYSFS_BASE "/sys/bus/iio/devices"
#define IIO_MAX_PATH   256

/* Varre /sys/bus/iio/devices procurando um device cujo arquivo "name"
 * contenha name_match (ex: "mpu6050"). Escreve o caminho completo do
 * diretório do device em devpath_out.
 * Retorna 0 em sucesso, -1 se não encontrado ou erro de leitura. 
 * */
int iio_find_device(const char *name_match, char *devpath_out, size_t out_len);

/* Lê um atributo numérico (double) de um arquivo dentro do devpath. */
int iio_read_attr_double(const char *devpath, const char *attr, double *out);

/* Lê um atributo inteiro (long). */
int iio_read_attr_long(const char *devpath, const char *attr, long *out);

#endif