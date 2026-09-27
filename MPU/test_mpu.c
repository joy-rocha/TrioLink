#include <stdio.h>
#include <unistd.h>

#include "./Inc/mpu_iio.h"

int main(void)
{
    mpu_iio_ctx_t ctx;

    if (mpu_iio_open(&ctx) != 0) {
        fprintf(stderr, "Erro ao inicializar MPU\n");
        return 1;
    }

    printf("MPU inicializado com sucesso!\n");

    mpu_sample_t sample;

    while (1) {

        if (mpu_iio_read(&ctx, &sample) == 0) {

            printf("\nTimestamp: %ld ms\n", sample.timestamp_ms);

            printf("ACCEL:\n");
            printf("X: %.3f | Y: %.3f | Z: %.3f\n",
                   sample.accel_x,
                   sample.accel_y,
                   sample.accel_z);

            printf("GYRO:\n");
            printf("X: %.3f | Y: %.3f | Z: %.3f\n",
                   sample.gyro_x,
                   sample.gyro_y,
                   sample.gyro_z);

        } else {
            fprintf(stderr, "Erro na leitura do MPU\n");
        }

        sleep(1);
    }

    return 0;
}