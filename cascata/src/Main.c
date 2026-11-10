/*
 * @file    Main.c
 * @brief   Integra os modulos do no de display: GPIO, botoes, MQTT (Fusion)
 *          e interface (BspScreen).
 */

#include "BspButton.h"
#include "BspGpio.h"
#include "BspScreen.h"
#include "Fusion.h"

#include <stdio.h>
#include <stdlib.h>

/// Broker MQTT padrao e porta
#define dMAIN_BROKER_HOST       "localhost"
#define dMAIN_BROKER_PORT       1883

/// Posicao dos argumentos opcionais: ./cascata [host] [porta]
#define dMAIN_ARG_HOST          1
#define dMAIN_ARG_PORT          2

int main(int argc, char **argv)
{
    int status = 1;
    const char *brokerHost = dMAIN_BROKER_HOST;
    int brokerPort = dMAIN_BROKER_PORT;

    if(argc > dMAIN_ARG_HOST)
    {
        brokerHost = argv[dMAIN_ARG_HOST];
    }

    if(argc > dMAIN_ARG_PORT)
    {
        brokerPort = atoi(argv[dMAIN_ARG_PORT]);
    }

    if(BspGpio_Init() == false)
    {
        (void)fprintf(stderr, "Falha ao iniciar o GPIO\n");
    }
    else if(BspButton_Init() == false)
    {
        (void)fprintf(stderr, "Falha ao iniciar os botoes\n");
    }
    else if(Fusion_Init(brokerHost, brokerPort) == false)
    {
        (void)fprintf(stderr, "Falha ao iniciar o MQTT\n");
    }
    else
    {
        status = BspScreen_Run(argc, argv);
        Fusion_Deinit();
    }

    BspGpio_Deinit();

    return status;
}