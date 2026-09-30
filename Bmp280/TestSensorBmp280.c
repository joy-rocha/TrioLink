/**************************************
* @file TestSensorBmp280.c
* @brief Stub de teste do SensorBmp280: injeta leituras falsas (sem tocar o
*        hardware) e imprime a ultima leitura e a media movel a cada chamada.
* @details Compile e rode junto com SensorBmp280.c e SimpleMovingAvg.c:
*   gcc TestSensorBmp280.c SensorBmp280.c SimpleMovingAvg.c -I. -lm -o teste
*   ./teste
* Este arquivo NAO faz parte do produto final: e so pra validar a logica de
* tratamento de dados sem precisar do Raspberry Pi nem do sensor de verdade.
***************************************/
#include "SensorBmp280.h"
#include <stdio.h>

/**************************************
/** @brief Simula uma leitura do sensor. Mesma assinatura que a
*         BspSensorBmp280_Read de verdade injetaria no Init.
* @details A cada chamada, a temperatura sobe 0,1 oC e a pressao cai 50 Pa,
*          soh pra ficar visivel, no print, que a media esta acompanhando o
*          valor instantaneo com atraso (e essa e a funcao de uma media
*          movel).
***************************************/
static bool StubReadSensor(s64 *timestampMs, s32 *temperatureMilliC, float *pressureKpa)
{
    static s64 fakeTimestamp = 1700000000000; // valor inicial qualquer
    static s32 fakeTemperatureMilliC = 25000; // 25,000 oC
    static float fakePressureKpa = 101.325f;  // 101,325 kPa = 101325 Pa

    fakeTimestamp += 1000;
    fakeTemperatureMilliC += 100;
    fakePressureKpa -= 0.05f;

    *timestampMs = fakeTimestamp;
    *temperatureMilliC = fakeTemperatureMilliC;
    *pressureKpa = fakePressureKpa;

    printf("  [STUB] leitura simulada: temp=%d milliC  pressure=%.3f kPa\n",
           fakeTemperatureMilliC, fakePressureKpa);

    return true;
}

int main(void)
{
    sensorBmp280Data_t last;
    sensorBmp280Data_t average;
    int i;

    printf("=== Teste SensorBmp280 ===\n\n");

    // 1) Init: so guarda o ponteiro da funcao de leitura e zera as medias.
    //    Nao le nada do sensor ainda.
    if(SensorBmp280_Init(StubReadSensor) != eSENSOR_BMP280_RETURN_OK)
    {
        printf("ERRO: SensorBmp280_Init falhou\n");
        return 1;
    }
    printf("SensorBmp280_Init OK\n\n");

    for(i = 1; i <= 20; i++)
    {
        printf("Chamada %d:\n", i);

        // 2) Update: eh quem de fato chama a funcao de leitura injetada,
        //    converte e alimenta a media movel.
        sensorBmp280Return_t updateResult = SensorBmp280_Update();

        if(updateResult != eSENSOR_BMP280_RETURN_OK)
        {
            printf("  -> SensorBmp280_Update falhou (codigo %d)\n\n", (int)updateResult);
            continue;
        }

        // 3) GetLastData / GetAverageData: eh quem entrega o resultado.
        SensorBmp280_GetLastData(&last);
        SensorBmp280_GetAverageData(&average);

        printf("  Ultima:  temp=%.2f oC  pressure=%.2f Pa  altitude=%.2f m\n",
               last.temperature, last.pressure, last.altitude);
        printf("  Media:   temp=%.2f oC  pressure=%.2f Pa  altitude=%.2f m\n\n",
               average.temperature, average.pressure, average.altitude);
    }

    return 0;
}
