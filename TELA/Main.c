/**************************************
* @file Main.c
* @addtogroup Main
* @brief Monitor de sensores (BMP280 e MPU6050) com exibicao no display TFT.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - cJSON
* - pthread
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
/// Caminho para o ficheiro de fonte TrueType
#define dMAIN_FONT_PATH           "Ubuntu-Regular.ttf"
/// Intervalo de atualizacao dos dados dos sensores em ms
#define dMAIN_UPDATE_INTERVAL_MS  1000
/// Tempo de espera do loop principal em us
#define dMAIN_LOOP_SLEEP_US       10000
/// Tempo de espera do modo debug em us
#define dMAIN_DEBUG_SLEEP_US      5000
/// Fator de conversao de segundos para ms
#define dMAIN_MS_PER_SEC          1000L
/// Fator de conversao de ns para ms
#define dMAIN_NS_PER_MS           1000000L

/**************************************
* CONSTANTES
***************************************/

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Variaveis de controlo do loop principal
static struct
{
    volatile sig_atomic_t isRunning;
} mainApp = {
    .isRunning = 1
};

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static void Main_SignalHandler(int signalNumber);
static long Main_MsNow(void);
static const char *Main_GetRawBmpJson(void);
static const char *Main_GetRawMpuJson(void);
static double Main_ExtractNumber(const cJSON *jsonObject, const char *key);
static void Main_CopyStatus(SensorData *sensorData, const cJSON *jsonObject);
static void Main_UpdateBmp(SensorData *sensorData);
static void Main_UpdateMpu(SensorData *sensorData);
static void Main_ShutdownSystem(void);
static int Main_ButtonDebug(void);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
/** @brief Ponto de entrada principal da aplicacao.
* @param argc: quantidade de argumentos.
* @param argv: vetor de argumentos da linha de comandos.
* @retval 0 em caso de sucesso, 1 em caso de erro.
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

    SensorData bmpData = { 0 };
    SensorData mpuData = { 0 };
    Screen currentScreen = SCR_HOME;
    int selectedIndex = 0;
    bool isDirty = true;
    long lastUpdateMs = -10000;

    while(mainApp.isRunning == 1)
    {
        long currentTimeMs = Main_MsNow();

        if((currentTimeMs - lastUpdateMs) >= dMAIN_UPDATE_INTERVAL_MS)
        {
            SensorData newBmpData;
            SensorData newMpuData;

            Main_UpdateBmp(&newBmpData);
            Main_UpdateMpu(&newMpuData);

            if((memcmp(&newBmpData, &bmpData, sizeof(bmpData)) != 0) ||
               (memcmp(&newMpuData, &mpuData, sizeof(mpuData)) != 0))
            {
                bmpData = newBmpData;
                mpuData = newMpuData;
                isDirty = true;
            }
            lastUpdateMs = currentTimeMs;
        }

        btnEvent_t event;
        while((event = Buttons_Poll()) != eBTN_EVENT_NONE)
        {
            if(event == eBTN_EVENT_NAV)
            {
                ui_next(currentScreen, &selectedIndex);
                isDirty = true;
            }
            else if(event == eBTN_EVENT_ENTER)
            {
                UiAction action;
                currentScreen = ui_enter(currentScreen, &selectedIndex, &action);
                isDirty = true;

                if(action == ACT_SHUTDOWN)
                {
                    Main_ShutdownSystem();
                    break;
                }
            }
        }

        if((isDirty == true) && (mainApp.isRunning == 1))
        {
            ui_render(currentScreen, selectedIndex, &bmpData, &mpuData);
            Lcd_Flush(fb);
            isDirty = false;
        }

        usleep(dMAIN_LOOP_SLEEP_US);
    }

    Buttons_Close();
    Lcd_Close();
    return 0;
}

/**************************************
* FUNCOES LOCAIS
***************************************/

/**************************************
/** @brief Trata os sinais de interrupcao e termino do sistema.
* @param signalNumber: numero do sinal recebido.
* @retval Nenhum.
***************************************/
static void Main_SignalHandler(int signalNumber)
{
    (void)signalNumber;
    mainApp.isRunning = 0;
}

/**************************************
/** @brief Retorna o tempo atual em milissegundos.
* @param Nenhum.
* @retval Tempo atual em ms.
***************************************/
static long Main_MsNow(void)
{
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    return (timeSpec.tv_sec * dMAIN_MS_PER_SEC) + (timeSpec.tv_nsec / dMAIN_NS_PER_MS);
}

/**************************************
/** @brief Retorna a string JSON simulada do sensor BMP.
* @param Nenhum.
* @retval String em formato JSON.
***************************************/
static const char *Main_GetRawBmpJson(void)
{
    return "{\"temperature\": 26.0, \"pressure\": 101300.0, \"altitude\": 550.0}";
}

/**************************************
/** @brief Retorna a string JSON simulada do sensor MPU.
* @param Nenhum.
* @retval String em formato JSON.
***************************************/
static const char *Main_GetRawMpuJson(void)
{
    return "{\"accel\": {\"x\": 10.0, \"y\": 5.0, \"z\": 12.0}}";
}

/**************************************
/** @brief Extrai um valor numerico de um objeto cJSON.
* @param jsonObject: ponteiro para o objeto cJSON.
* @param key: chave do atributo a ser extraido.
* @retval Valor numerico do atributo ou 0.0 caso nao exista.
***************************************/
static double Main_ExtractNumber(const cJSON *jsonObject, const char *key)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(jsonObject, key);
    return cJSON_IsNumber(item) ? item->valuedouble : 0.0;
}

/**************************************
/** @brief Copia o campo de estado do JSON para a estrutura SensorData.
* @param sensorData: ponteiro para a estrutura de destino.
* @param jsonObject: ponteiro para o objeto cJSON de origem.
* @retval Nenhum.
***************************************/
static void Main_CopyStatus(SensorData *sensorData, const cJSON *jsonObject)
{
    const cJSON *statusItem = cJSON_GetObjectItemCaseSensitive(jsonObject, "estado");
    snprintf(sensorData->estado, sizeof(sensorData->estado), "%s", cJSON_IsString(statusItem) ? statusItem->valuestring : "--");
}

/**************************************
/** @brief Atualiza a estrutura de dados do sensor BMP.
* @param sensorData: ponteiro para a estrutura a ser preenchida.
* @retval Nenhum.
***************************************/
static void Main_UpdateBmp(SensorData *sensorData)
{
    memset(sensorData, 0, sizeof(*sensorData));
    cJSON *jsonProcessed = DecodeBmp_ProcessData(Main_GetRawBmpJson());

    if(jsonProcessed == NULL)
    {
        strcpy(sensorData->estado, "--");
        return;
    }

    sensorData->online = true;
    sensorData->v1 = Main_ExtractNumber(jsonProcessed, "pressao");
    sensorData->v2 = Main_ExtractNumber(jsonProcessed, "temperatura");
    sensorData->v3 = Main_ExtractNumber(jsonProcessed, "altitude");
    Main_CopyStatus(sensorData, jsonProcessed);
    cJSON_Delete(jsonProcessed);
}

/**************************************
/** @brief Atualiza a estrutura de dados do sensor MPU.
* @param sensorData: ponteiro para a estrutura a ser preenchida.
* @retval Nenhum.
***************************************/
static void Main_UpdateMpu(SensorData *sensorData)
{
    memset(sensorData, 0, sizeof(*sensorData));
    cJSON *jsonProcessed = DecodeMpu_ProcessData(Main_GetRawMpuJson());

    if(jsonProcessed == NULL)
    {
        strcpy(sensorData->estado, "--");
        return;
    }

    sensorData->online = true;
    sensorData->v1 = Main_ExtractNumber(jsonProcessed, "direcao");
    sensorData->v2 = Main_ExtractNumber(jsonProcessed, "velocidade");
    Main_CopyStatus(sensorData, jsonProcessed);
    cJSON_Delete(jsonProcessed);
}

/**************************************
/** @brief Exibe mensagem de encerramento e executa o comando de shutdown.
* @param Nenhum.
* @retval Nenhum.
***************************************/
static void Main_ShutdownSystem(void)
{
    gfx_clear(rgb(18, 18, 20));
    gfx_text(dLCD_W / 2, dLCD_H / 2, 20, "DESLIGANDO...", rgb(235, 235, 240), ALIGN_CENTER);
    Lcd_Flush(fb);

    int systemResult = system("sudo /sbin/shutdown -h now");
    (void)systemResult;
    mainApp.isRunning = 0;
}

/**************************************
/** @brief Loop de teste e depuracao dos botoes.
* @param Nenhum.
* @retval 0 apos conclusao.
***************************************/
static int Main_ButtonDebug(void)
{
    printf("Aperte os botoes (Ctrl+C sai)\n");
    while(mainApp.isRunning == 1)
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
        usleep(dMAIN_DEBUG_SLEEP_US);
    }
    return 0;
}
/** @} DOXYGEN GROUP TAG END OF FILE */