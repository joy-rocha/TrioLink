/**************************************
* @file Main.c
* @addtogroup Main
* @brief Monitor de sensores (BMP280 + MPU6050) em display TFT com botoes.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - cJSON
* - FreeType
* - lgpio
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 05/11/2026</b> \n Assert \n Primeira versao adaptada.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "Buttons.h"
#include "Decoders.h"
#include "Gfx.h"
#include "Lcd.h"
#include "Ui.h"

#include <cjson/cJSON.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Caminho do ficheiro de fonte padrao da interface
#define dMAIN_FONT_PATH        "Ubuntu-Regular.ttf"
/// Intervalo de amostragem dos sensores em milissegundos
#define dMAIN_UPDATE_INTERVAL_MS 1000
/// Fator de conversao de segundos para milissegundos
#define dMAIN_MS_PER_SEC       1000L
/// Fator de conversao de nanossegundos para milissegundos
#define dMAIN_NS_PER_MS        1000000L

/**************************************
* CONSTANTES
***************************************/

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Variavel global de sinalizacao de execucao
static volatile sig_atomic_t mainIsRunning = 1;

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static void Main_SignalHandler(int signalNumber);
static long Main_MsNow(void);
static const char *Main_GetRawBmpJson(void);
static const char *Main_GetRawMpuJson(void);
static double Main_ExtractJsonNumber(const cJSON *jsonObject, const char *keyName);
static void Main_CopyStatusString(sensorData_t *sensorDataPtr, const cJSON *jsonObject);
static void Main_UpdateBmpData(sensorData_t *sensorDataPtr);
static void Main_UpdateMpuData(sensorData_t *sensorDataPtr);
static void Main_ShutdownSystem(void);
static int Main_ButtonDebug(void);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
* @brief Ponto de entrada principal da aplicacao.
* @param argc: quantidade de argumentos da linha de comando.
* @param argv: array de strings dos argumentos.
* @retval 0 em caso de encerramento normal, 1 em caso de erro.
***************************************/
int main(int argc, char **argv)
{
    signal(SIGINT, Main_SignalHandler);
    signal(SIGTERM, Main_SignalHandler);

    if(Gfx_Init(dMAIN_FONT_PATH) < 0)
    {
        return 1;
    }

    if(Lcd_Init() < 0)
    {
        return 1;
    }

    if(Buttons_Init() < 0)
    {
        Lcd_Close();
        return 1;
    }

    if((argc > 1) && (strcmp(argv[1], "--btn-debug") == 0))
    {
        Main_ButtonDebug();
        Buttons_Close();
        Lcd_Close();
        return 0;
    }

    sensorData_t bmpData = {0};
    sensorData_t mpuData = {0};
    screen_t currentScreen = eUI_SCREEN_HOME;
    int selectedIndex = 0;
    bool needRedraw = true;
    long lastUpdateTimeMs = -10000;

    while(mainIsRunning == 1)
    {
        long currentTimeMs = Main_MsNow();

        if((currentTimeMs - lastUpdateTimeMs) >= dMAIN_UPDATE_INTERVAL_MS)
        {
            sensorData_t newBmpData;
            sensorData_t newMpuData;

            Main_UpdateBmpData(&newBmpData);
            Main_UpdateMpuData(&newMpuData);

            if((memcmp(&newBmpData, &bmpData, sizeof(sensorData_t)) != 0) ||
               (memcmp(&newMpuData, &mpuData, sizeof(sensorData_t)) != 0))
            {
                bmpData = newBmpData;
                mpuData = newMpuData;
                needRedraw = true;
            }

            lastUpdateTimeMs = currentTimeMs;
        }

        btnEvent_t event;
        while((event = Buttons_Poll()) != eBTN_EVENT_NONE)
        {
            if(event == eBTN_EVENT_NAV)
            {
                Ui_Next(currentScreen, &selectedIndex);
                needRedraw = true;
            }
            else if(event == eBTN_EVENT_ENTER)
            {
                uiAction_t actionExecuted;
                currentScreen = Ui_Enter(currentScreen, &selectedIndex, &actionExecuted);
                needRedraw = true;

                if(actionExecuted == eUI_ACTION_SHUTDOWN)
                {
                    Main_ShutdownSystem();
                    break;
                }
            }
        }

        if((needRedraw == true) && (mainIsRunning == 1))
        {
            Ui_Render(currentScreen, selectedIndex, &bmpData, &mpuData);
            Lcd_Flush(frameBuffer);
            needRedraw = false;
        }

        usleep(10000);
    }

    Buttons_Close();
    Lcd_Close();
    return 0;
}

/**************************************
* FUNCOES LOCAIS
***************************************/

/**************************************
* @brief Trata os sinais de interrupcao do sistema.
* @param signalNumber: numero do sinal recebido.
* @retval Nenhum.
***************************************/
static void Main_SignalHandler(int signalNumber)
{
    (void)signalNumber;
    mainIsRunning = 0;
}

/**************************************
* @brief Obtem o tempo monotonic atual em milissegundos.
* @param Nenhum.
* @retval Tempo atual em milissegundos.
***************************************/
static long Main_MsNow(void)
{
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    return (timeSpec.tv_sec * dMAIN_MS_PER_SEC) + (timeSpec.tv_nsec / dMAIN_NS_PER_MS);
}

/**************************************
* @brief Retorna a string JSON simulada do sensor BMP280.
* @param Nenhum.
* @retval Ponteiro para string JSON estatica.
***************************************/
static const char *Main_GetRawBmpJson(void)
{
    return "{\"temperature\": 26.0, \"pressure\": 101300.0, \"altitude\": 550.0}";
}

/**************************************
* @brief Retorna a string JSON simulada do sensor MPU6050.
* @param Nenhum.
* @retval Ponteiro para string JSON estatica.
***************************************/
static const char *Main_GetRawMpuJson(void)
{
    return "{\"accel\": {\"x\": 10.0, \"y\": 5.0, \"z\": 12.0}}";
}

/**************************************
* @brief Extrai um valor numerico de uma chave do objeto cJSON.
* @param jsonObject: objeto JSON.
* @param keyName: nome da chave.
* @retval Valor numerico extraido ou 0.0 em caso de falha.
***************************************/
static double Main_ExtractJsonNumber(const cJSON *jsonObject, const char *keyName)
{
    const cJSON *itemPtr = cJSON_GetObjectItemCaseSensitive(jsonObject, keyName);
    return (cJSON_IsNumber(itemPtr) != 0) ? itemPtr->valuedouble : 0.0;
}

/**************************************
* @brief Copia a string do campo 'estado' do cJSON para a estrutura local.
* @param sensorDataPtr: ponteiro para a estrutura de destino.
* @param jsonObject: objeto cJSON processado.
* @retval Nenhum.
***************************************/
static void Main_CopyStatusString(sensorData_t *sensorDataPtr, const cJSON *jsonObject)
{
    const cJSON *statusItem = cJSON_GetObjectItemCaseSensitive(jsonObject, "estado");
    snprintf(sensorDataPtr->statusStr, sizeof(sensorDataPtr->statusStr), "%s",
             (cJSON_IsString(statusItem) != 0) ? statusItem->valuestring : "--");
}

/**************************************
* @brief Atualiza a estrutura de dados com a leitura decodificada do BMP280.
* @param sensorDataPtr: ponteiro para a estrutura do sensor.
* @retval Nenhum.
***************************************/
static void Main_UpdateBmpData(sensorData_t *sensorDataPtr)
{
    memset(sensorDataPtr, 0, sizeof(sensorData_t));
    cJSON *parsedJson = DecodeBmp_ProcessData(Main_GetRawBmpJson());

    if(parsedJson == NULL)
    {
        strcpy(sensorDataPtr->statusStr, "--");
        return;
    }

    sensorDataPtr->isOnline = true;
    sensorDataPtr->val1 = Main_ExtractJsonNumber(parsedJson, "pressao");
    sensorDataPtr->val2 = Main_ExtractJsonNumber(parsedJson, "temperatura");
    sensorDataPtr->val3 = Main_ExtractJsonNumber(parsedJson, "altitude");
    Main_CopyStatusString(sensorDataPtr, parsedJson);

    cJSON_Delete(parsedJson);
}

/**************************************
* @brief Atualiza a estrutura de dados com a leitura decodificada do MPU6050.
* @param sensorDataPtr: ponteiro para a estrutura do sensor.
* @retval Nenhum.
***************************************/
static void Main_UpdateMpuData(sensorData_t *sensorDataPtr)
{
    memset(sensorDataPtr, 0, sizeof(sensorData_t));
    cJSON *parsedJson = DecodeMpu_ProcessData(Main_GetRawMpuJson());

    if(parsedJson == NULL)
    {
        strcpy(sensorDataPtr->statusStr, "--");
        return;
    }

    sensorDataPtr->isOnline = true;
    sensorDataPtr->val1 = Main_ExtractJsonNumber(parsedJson, "direcao");
    sensorDataPtr->val2 = Main_ExtractJsonNumber(parsedJson, "velocidade");
    Main_CopyStatusString(sensorDataPtr, parsedJson);

    cJSON_Delete(parsedJson);
}

/**************************************
* @brief Exibe mensagem de desligamento no LCD e envia o comando de shutdown.
* @param Nenhum.
* @retval Nenhum.
***************************************/
static void Main_ShutdownSystem(void)
{
    Gfx_Clear(Gfx_Rgb(18, 18, 20));
    Gfx_Text(dLCD_W / 2, dLCD_H / 2, 20, "DESLIGANDO...", Gfx_Rgb(235, 235, 240), eGFX_ALIGN_CENTER);
    Lcd_Flush(frameBuffer);

    int result = system("sudo /sbin/shutdown -h now");
    (void)result;
    mainIsRunning = 0;
}

/**************************************
* @brief Modo de teste e depuracao dos botoes fisicos.
* @param Nenhum.
* @retval 0 ao finalizar.
***************************************/
static int Main_ButtonDebug(void)
{
    printf("Aperte os botoes (Ctrl+C sai)\n");
    while(mainIsRunning == 1)
    {
        btnEvent_t event = Buttons_Poll();
        if(event == eBTN_EVENT_NAV)
        {
            printf("PRETO    -> navegar\n");
        }
        else if(event == eBTN_EVENT_ENTER)
        {
            printf("VERMELHO -> enter\n");
        }
        fflush(stdout);
        usleep(5000);
    }
    return 0;
}
/** @} DOXYGEN GROUP TAG END OF FILE */