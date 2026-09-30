/**************************************
* @file JsonBmp280.c
* @addtogroup JsonBmp280
* @brief Monta o JSON do topico sensor/bmp/raw a partir de um payload ja
*        tratado, recebido por parametro.
* @author Luis EDuardo
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - AssertTypes;
* - cJSON (Raspberry Pi OS: pacote libcjson-dev, link com -lcjson);
* - Biblioteca matematica (-lm), por causa do round.
* \n <b>Observacoes:</b>
* - Este modulo nao conhece o SensorBmp280 nem nenhuma outra fonte de dados:
*   ele so sabe transformar um jsonBmp280Payload_t em texto JSON. Quem chama
*   e quem decide de onde vem o payload (ver regra 4.13 das normas: mesmo
*   principio de injecao de dependencia, aqui aplicado a dados em vez de
*   ponteiros de funcao). Isso permite reaproveitar este arquivo em outro
*   projeto, ou serializar dados de origem diferente, sem alterar uma linha;
* - Este modulo apenas gera o texto JSON: nao envia nada. O envio (MQTT) e
*   responsabilidade de outro modulo;
* - Formato gerado (campos na ordem do schema):
*   {"timestamp":1789335000123,"temperature":25.43,"pressure":101325.2,
*   "altitude":12.7};
* - O timestamp e o da aquisicao da leitura, conforme o schema, e nao o do
*   envio;
* - Os valores sao arredondados antes de entrar no JSON, pois o cJSON guarda
*   numeros como double e, sem arredondar, um float como 25.43 apareceria com
*   varias casas espurias (ex.: 25.430000305175781).
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "JsonBmp280.h"
#include <cjson/cJSON.h>
#include <math.h>
#include <string.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Fator para arredondar em 1 casa decimal
#define dSCALE_1_DECIMAL 10.0
/// Fator para arredondar em 2 casas decimais
#define dSCALE_2_DECIMALS 100.0

/**************************************
* CONSTANTES
***************************************/
/// Nome do campo timestamp no schema sensor/bmp/raw
static const char keyTimestamp[] = "timestamp";
/// Nome do campo temperature no schema sensor/bmp/raw
static const char keyTemperature[] = "temperature";
/// Nome do campo pressure no schema sensor/bmp/raw
static const char keyPressure[] = "pressure";
/// Nome do campo altitude no schema sensor/bmp/raw
static const char keyAltitude[] = "altitude";

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static double JsonBmp280_Round(float value, double scale);
static bool JsonBmp280_FillObject(cJSON *root, const jsonBmp280Payload_t *payload);

/**************************************
* FUNCOES PUBLICAS
***************************************/
/**************************************
/** @brief Gera o JSON do topico sensor/bmp/raw a partir de um payload ja
*         tratado.
* @param payload: dados a serem serializados, no formato do schema.
* @param outBuffer: buffer que recebe o texto JSON (terminado em nulo).
* @param outBufferSize: tamanho do buffer, em bytes (use
*        dJSON_BMP280_BUFFER_SIZE).
* @param outLength: ponteiro que recebe o tamanho do JSON gerado, em bytes,
*        sem contar o terminador nulo.
* @retval eJSON_BMP280_RETURN_OK em caso de sucesso;
*         eJSON_BMP280_RETURN_INVALID_ARGUMENT se algum ponteiro for nulo ou o
*         tamanho do buffer for zero;
*         eJSON_BMP280_RETURN_BUFFER_TOO_SMALL se o JSON nao coube;
*         eJSON_BMP280_RETURN_ERROR se o cJSON falhou.
* @details Cria um objeto cJSON na memoria, preenche os quatro campos,
*          imprime o texto no buffer do chamador e libera o objeto. Como o
*          texto vai para o buffer do chamador, ele nao precisa dar free em
*          nada. Quem chama e quem obtem o payload (ex.: de
*          SensorBmp280_GetAverageData); esta funcao nao busca dado nenhum
*          por conta propria.
***************************************/
jsonBmp280Return_t JsonBmp280_Build(const jsonBmp280Payload_t *payload, char *outBuffer, u16 outBufferSize, u16 *outLength)
{
    jsonBmp280Return_t result = eJSON_BMP280_RETURN_OK;
    cJSON *root = dNULL;

    if((payload == dNULL) || (outBuffer == dNULL) || (outBufferSize == 0) || (outLength == dNULL))
    {
        result = eJSON_BMP280_RETURN_INVALID_ARGUMENT;
    }
    else
    {
        root = cJSON_CreateObject();

        if(root == dNULL)
        {
            result = eJSON_BMP280_RETURN_ERROR;
        }
        else
        {
            if(JsonBmp280_FillObject(root, payload) == false)
            {
                result = eJSON_BMP280_RETURN_ERROR;
            }
            // Ultimo parametro false = JSON compacto (sem quebras de linha)
            else if(cJSON_PrintPreallocated(root, outBuffer, (int)outBufferSize, false) == false)
            {
                result = eJSON_BMP280_RETURN_BUFFER_TOO_SMALL;
            }
            else
            {
                *outLength = (u16)strlen(outBuffer);
            }

            // Libera o objeto criado; o texto ja esta no buffer do chamador
            cJSON_Delete(root);
        }
    }

    return result;
}

/**************************************
* FUNCOES LOCAIS
***************************************/
/**************************************
/** @brief Arredonda um valor para o numero de casas decimais do schema.
* @param value: valor a ser arredondado.
* @param scale: 10 elevado ao numero de casas (ex.: 100.0 para 2 casas).
* @retval Valor arredondado, em double (tipo que o cJSON usa).
***************************************/
static double JsonBmp280_Round(float value, double scale)
{
    return (round((double)value * scale) / scale);
}

/**************************************
/** @brief Adiciona ao objeto cJSON os quatro campos do schema
*         sensor/bmp/raw.
* @param root: objeto cJSON ja criado.
* @param payload: dados a serem gravados no objeto.
* @retval true se todos os campos foram adicionados; false se algum falhou.
* @details O timestamp e inteiro (int64 no schema); temperatura tem 2 casas
*          decimais; pressao e altitude tem 1 casa, como nos exemplos do
*          schema.
***************************************/
static bool JsonBmp280_FillObject(cJSON *root, const jsonBmp280Payload_t *payload)
{
    bool isValid = false;

    if((cJSON_AddNumberToObject(root, keyTimestamp, (double)payload->timestamp) != dNULL) &&
      (cJSON_AddNumberToObject(root, keyTemperature, JsonBmp280_Round(payload->temperature, dSCALE_2_DECIMALS)) != dNULL) &&
      (cJSON_AddNumberToObject(root, keyPressure, JsonBmp280_Round(payload->pressure, dSCALE_1_DECIMAL)) != dNULL) &&
      (cJSON_AddNumberToObject(root, keyAltitude, JsonBmp280_Round(payload->altitude, dSCALE_1_DECIMAL)) != dNULL))
    {
        isValid = true;
    }

    return isValid;
}

/** @} DOXYGEN GROUP TAG END OF FILE */
