#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"
#include "Decoders.h"


int main() {
    // 1. Simulação dos JSONs brutos vindos dos sensores
    const char *json_bruto_bmp = "{\"temperature\": 26.0, \"pressure\": 101300.0, \"altitude\": 550.0}";
    const char *json_bruto_mpu = "{\"accel\": {\"x\": 10.0, \"y\": 5.0, \"z\": 12.0}}";

    // 2. Processa os dados usando suas funções
    cJSON *bmp_processado = processar_dados_bmp(json_bruto_bmp);
    cJSON *mpu_processado = processar_dados_mpu(json_bruto_mpu);

    // 3. Agrupa tudo em um único JSON principal
    cJSON *raiz = cJSON_CreateObject();
    cJSON_AddItemToObject(raiz, "bmp", bmp_processado);
    cJSON_AddItemToObject(raiz, "mpu", mpu_processado);

    // 4. Converte para string e imprime no STDOUT (para o Python ler)
    char *json_saida = cJSON_PrintUnformatted(raiz);
    printf("%s\n", json_saida);

    // 5. Libera a memória
    free(json_saida);
    cJSON_Delete(raiz);

    return 0;
}