#include "mpu_iio.h"
#include "iio_sysfs.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

static int64_t now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int mpu_iio_open(mpu_iio_ctx_t *ctx) {
    memset(ctx, 0, sizeof(*ctx));

    //Encontrar o sensor
    if (iio_find_device("mpu6500", ctx->devpath, sizeof(ctx->devpath)) != 0) {
        fprintf(stderr, "mpu_iio: device nao encontrado — overlay/driver carregado?\n");
        return -1;
    }

    //Scale sensor
    if (iio_read_attr_double(ctx->devpath, "in_accel_scale", &ctx->accel_scale) != 0) {
        fprintf(stderr, "mpu_iio: falha ao ler in_accel_scale\n");
        return -1;
    }

    if (iio_read_attr_double(ctx->devpath, "in_anglvel_scale", &ctx->gyro_scale) != 0) {
        fprintf(stderr, "mpu_iio: falha ao ler in_anglvel_scale\n");
        return -1;
    }

    /* Offset é opcional dependendo da versão do driver — não é erro fatal
     * se não existir, só desativamos a correção. */
    long ox = 0, oy = 0, oz = 0;
    int ok = 1;
    ok &= (iio_read_attr_long(ctx->devpath, "in_accel_x_offset", &ox) == 0);
    ok &= (iio_read_attr_long(ctx->devpath, "in_accel_y_offset", &oy) == 0);
    ok &= (iio_read_attr_long(ctx->devpath, "in_accel_z_offset", &oz) == 0);
    ctx->has_offsets = ok;
    if (ok) {
        ctx->accel_offset_x = ox;
        ctx->accel_offset_y = oy;
        ctx->accel_offset_z = oz;
    }

    return 0;
}

static int read_axis(const char *devpath, const char *type, char axis,
                      double scale, long offset, int has_offset, float *out) {
    char attr[64];
    snprintf(attr, sizeof(attr), "in_%s_%c_raw", type, axis);

    long raw;
    if (iio_read_attr_long(devpath, attr, &raw) != 0)
        return -1;

    double physical = has_offset ? (raw + offset) * scale : raw * scale;
    *out = (float)physical;
    return 0;
}

int mpu_iio_read(mpu_iio_ctx_t *ctx, mpu_sample_t *sample) {
    sample->timestamp_ms = now_ms();

    if (read_axis(ctx->devpath, "accel", 'x', ctx->accel_scale, ctx->accel_offset_x, ctx->has_offsets, &sample->accel_x) != 0) return -1;
    if (read_axis(ctx->devpath, "accel", 'y', ctx->accel_scale, ctx->accel_offset_y, ctx->has_offsets, &sample->accel_y) != 0) return -1;
    if (read_axis(ctx->devpath, "accel", 'z', ctx->accel_scale, ctx->accel_offset_z, ctx->has_offsets, &sample->accel_z) != 0) return -1;

    if (read_axis(ctx->devpath, "anglvel", 'x', ctx->gyro_scale, 0, 0, &sample->gyro_x) != 0) return -1;
    if (read_axis(ctx->devpath, "anglvel", 'y', ctx->gyro_scale, 0, 0, &sample->gyro_y) != 0) return -1;
    if (read_axis(ctx->devpath, "anglvel", 'z', ctx->gyro_scale, 0, 0, &sample->gyro_z) != 0) return -1;

    return 0;
}