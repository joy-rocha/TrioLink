#include <stdio.h>
#include <stdlib.h>
#include <math.h>  // usa pra fazer os calculos pra convereter os numeros
#include "cJSON.h"

// Função modular para processar APENAS o MPU6050
cJSON* processar_dados_mpu(const char* json_amigo_mpu) { // formata o json bruto pra eu poder ler bonitinho
    // 1. Recebe e lê o JSON bruto
    cJSON *json_in = cJSON_Parse(json_amigo_mpu); 
    if (json_in == NULL) {
        printf("Erro ao ler o JSON do MPU.\n");
        return NULL;
    }

    // 2. Extrai as coordenadas X, Y, Z (Acelerômetro)
    // Entra primeiro no objeto "accel" especificado no schema
    cJSON *accel = cJSON_GetObjectItemCaseSensitive(json_in, "accel");
    if (accel == NULL) {
        printf("Objeto 'accel' ausente no JSON do MPU.\n");
        cJSON_Delete(json_in);
        return NULL;
    }

    // Agora extrai x, y e z de dentro do objeto accel
    cJSON *accel_x = cJSON_GetObjectItemCaseSensitive(accel, "x"); 
    cJSON *accel_y = cJSON_GetObjectItemCaseSensitive(accel, "y");
    cJSON *accel_z = cJSON_GetObjectItemCaseSensitive(accel, "z");

    // 3. MATEMÁTICA: Convertendo coordenadas 3D para dados da Interface
    // Calcula a Direção (Ângulo em graus usando X e Y), atan2 retorna em radianos, multiplicamos por (180 / PI) para ter em graus
    double direcao_graus = atan2(accel_y->valuedouble, accel_x->valuedouble) * (180.0 / 3.14159265);
    if (direcao_graus < 0) direcao_graus += 360.0; // Mantém entre 0 e 360 graus

    // Calcula a "Intensidade" do movimento (Magnitude do vetor 3D), formula: √(X² + Y² + Z²)
    double aceleracao_total = sqrt(pow(accel_x->valuedouble, 2) + pow(accel_y->valuedouble, 2) + pow(accel_z->valuedouble, 2));

    /* Nota: Para ter VELOCIDADE real (m/s), você precisaria multiplicar a aceleração 
       pelo tempo (delta T) entre as leituras. Como aqui estamos lendo um frame único, 
       vamos enviar a magnitude da aceleração para a chave "velocidade" da sua interface. */

    // 4. Cria o JSON refinado para o Python ler
    cJSON *json_out = cJSON_CreateObject();
    cJSON_AddStringToObject(json_out, "status", "online"); // adcionando um campo no json
    cJSON_AddNumberToObject(json_out, "direcao", round(direcao_graus * 10) / 10); // Arredondando os valores pra ficar bonito na tela
    cJSON_AddNumberToObject(json_out, "velocidade", round(aceleracao_total * 10) / 10);

    // 5. REGRA DE ESTADO (Definindo o que é Anormal)
    if (aceleracao_total > 15.0) {
        cJSON_AddStringToObject(json_out, "estado", "anormal");
    } else {
        cJSON_AddStringToObject(json_out, "estado", "normal");
    }

    // Libera a memória do JSON bruto
    cJSON_Delete(json_in);

    return json_out;
}