#ifndef DECODERS_H
#define DECODERS_H
#include "cJSON.h"

cJSON* processar_dados_mpu(const char* json_amigo_mpu);
cJSON* processar_dados_bmp(const char* json_amigo_bmp);

#endif