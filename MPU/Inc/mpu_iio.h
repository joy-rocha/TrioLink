#ifndef MPU_IIO_H
#define MPU_IIO_H

#include <stdint.h>
#include "iio_sysfs.h"

/* Amostra já convertida para unidades físicas (m/s² e rad/s — as mesmas
 * unidades definidas no Schemas.md do projeto, sem conversão extra
 * antes de montar o payload MQTT). */
typedef struct {
    int64_t timestamp_ms; /* gerado no host, Unix ms UTC */
    float accel_x, accel_y, accel_z; /* m/s² */
    float gyro_x,  gyro_y,  gyro_z;  /* rad/s */
} mpu_sample_t;

/* Contexto do sensor: caminho descoberto + escalas/offsets cacheados
 * na abertura, para não reler arquivos que não mudam a cada amostra. */
typedef struct {
    char devpath[IIO_MAX_PATH];
    double accel_scale;
    double gyro_scale;
    long accel_offset_x, accel_offset_y, accel_offset_z;
    int has_offsets;
} mpu_iio_ctx_t;

/* Localiza o device, valida se os arquivos esperados existem, e
 * cacheia scale/offset. Retorna 0 em sucesso, -1 em erro. */
int mpu_iio_open(mpu_iio_ctx_t *ctx);

/* Lê uma amostra completa (accel + gyro). Retorna 0 em sucesso,
 * -1 se qualquer leitura de atributo falhar. */
int mpu_iio_read(mpu_iio_ctx_t *ctx, mpu_sample_t *sample);

#endif