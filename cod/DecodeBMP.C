#include <stdio.h>
#include <stdlib.h>
#include "Decoders.h"

// Função modular para processar APENAS o BMP280
cJSON* processar_dados_bmp(const char* json_amigo_bmp) {
    // 1. Recebe e lê o JSON bruto
    cJSON *json_in = cJSON_Parse(json_amigo_bmp);
    if (json_in == NULL) {
        printf("Erro ao ler o JSON do BMP.\n");
        return NULL;
    }

    // 2. Extrai os valores diretamente da raiz do JSON, usando os nomes exatos do Schema
    cJSON *temp = cJSON_GetObjectItemCaseSensitive(json_in, "temperature");
    cJSON *press = cJSON_GetObjectItemCaseSensitive(json_in, "pressure");
    cJSON *alt = cJSON_GetObjectItemCaseSensitive(json_in, "altitude");

    // Verifica se os itens realmente existem para evitar crash
    if (temp == NULL || press == NULL || alt == NULL) {
        printf("Faltam dados essenciais no JSON do BMP.\n");
        cJSON_Delete(json_in);
        return NULL;
    }

    // 3. Cria o JSON refinado para o Python ler (com conversão Pa -> hPa)
    cJSON *json_out = cJSON_CreateObject();
    cJSON_AddStringToObject(json_out, "status", "online");
    cJSON_AddNumberToObject(json_out, "temperatura", temp->valuedouble);
    cJSON_AddNumberToObject(json_out, "pressao", press->valuedouble / 100.0); 
    cJSON_AddNumberToObject(json_out, "altitude", alt->valuedouble);

    // 4. REGRA DE ESTADO (Definindo o que é Anormal)
    // Usando 57°C como limite de temperatura anormal conforme você solicitou antes
    if (temp->valuedouble >= 57.0) {
        cJSON_AddStringToObject(json_out, "estado", "anormal");
    } else {
        cJSON_AddStringToObject(json_out, "estado", "normal");
    }

    // Libera a memória do JSON bruto
    cJSON_Delete(json_in);

    return json_out;
}