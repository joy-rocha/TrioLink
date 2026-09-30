/**************************************
* @file TestBspSensorBmp280.c
* @brief Teste de integracao real: usa a BspSensorBmp280 pra achar e ler o
*        BMP280 pelo kernel (sysfs/IIO) e o SensorBmp280 pra tratar os dados.
* @details Este teste PRECISA rodar no Raspberry Pi 5 de verdade, com o
*          sensor conectado por I2C e o overlay de device tree habilitado
*          (dtoverlay=i2c-sensor,bmp280 no config.txt). Nao roda num PC
*          qualquer, porque depende de /sys/bus/iio/devices existir.
* Compile com:
*   gcc TestBspSensorBmp280.c BspSensorBmp280.c SensorBmp280.c SimpleMovingAvg.c -I. -lm -o teste_bmp280
* Rode com:
*   ./teste_bmp280
***************************************/
#include "BspSensorBmp280.h"
#include "SensorBmp280.h"
#include <stdio.h>
#include <unistd.h>

int main(void)
{
    sensorBmp280Data_t last;
    sensorBmp280Data_t average;
    bspSensorBmp280Return_t bspResult;
    int i;

    printf("=== Teste BspSensorBmp280 + SensorBmp280 ===\n\n");

    // 1) Bsp_Init: varre /sys/bus/iio/devices ate achar o BMP280 e injeta a
    //    funcao de leitura real (a que le os arquivos do kernel) no
    //    SensorBmp280.
    bspResult = BspSensorBmp280_Init();

    if(bspResult != eBSP_SENSOR_BMP280_RETURN_OK)
    {
        printf("ERRO: BspSensorBmp280_Init falhou (codigo %d)\n", (int)bspResult);
        printf("Verifique se o overlay do BMP280 esta habilitado no config.txt\n");
        printf("e confira com: cat /sys/bus/iio/devices/iio:device0/name\n");
        return 1;
    }

    printf("BspSensorBmp280_Init OK: sensor encontrado no kernel\n\n");

    while(1)
    {
        sensorBmp280Return_t updateResult;

        printf("Leitura %d:\n", i);

        // 2) Update: chama de fato a leitura (via sysfs), converte unidades
        //    e alimenta a media movel.
        updateResult = SensorBmp280_Update();

        if(updateResult != eSENSOR_BMP280_RETURN_OK)
        {
            printf("  -> SensorBmp280_Update falhou (codigo %d)\n\n", (int)updateResult);
        }
        else
        {
            // 3) GetLastData / GetAverageData: pega o resultado.
            SensorBmp280_GetLastData(&last);
            SensorBmp280_GetAverageData(&average);

            printf("  Ultima:  temp=%.2f oC  pressure=%.2f Pa  altitude=%.2f m\n",
                   last.temperature, last.pressure, last.altitude);
            printf("  Media:   temp=%.2f oC  pressure=%.2f Pa  altitude=%.2f m\n\n",
                   average.temperature, average.pressure, average.altitude);
        }

        sleep(1);
    }

    return 0;
}
