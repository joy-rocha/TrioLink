/**************************************
* @file Buttons.c
* @addtogroup Buttons
* @brief Leitura de push-buttons com debounce e fila em thread separada.
* @author Assert
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - lgpio
* - pthread
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 05/11/2026</b> \n Assert \n Primeira versao adaptada.
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "Buttons.h"
#include <lgpio.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Chip GPIO padrao da Raspberry Pi
#define dBUTTONS_GPIOCHIP    0
/// Tempo de debounce em milissegundos
#define dBUTTONS_DEBOUNCE_MS 30
/// Intervalo de amostragem da thread em microssegundos
#define dBUTTONS_POLL_US     2000
/// Tamanho da fila circular de eventos
#define dBUTTONS_QSIZE       16
/// Fator de conversao de segundos para milissegundos
#define dBUTTONS_MS_PER_SEC  1000L
/// Fator de conversao de nanossegundos para milissegundos
#define dBUTTONS_NS_PER_MS   1000000L
/// Quantidade de botoes monitorados
#define dBUTTONS_COUNT       2

/**************************************
* CONSTANTES
***************************************/

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Estrutura de estado individual do botao
typedef struct
{
    int pin;
    btnEvent_t event;
    int stableState;
    int rawState;
    long lastChangeTimeMs;
} buttonState_t;

/// Estrutura do modulo de botoes
static struct
{
    buttonState_t btnList[dBUTTONS_COUNT];
    int gpioHandle;
    pthread_t threadId;
    volatile int isRunning;
    pthread_mutex_t mutex;
    btnEvent_t eventQueue[dBUTTONS_QSIZE];
    int queueHead;
    int queueTail;
} buttons = {
    .btnList = {
        { dBUTTONS_PIN_NAV, eBTN_EVENT_NAV, 1, 1, 0 },
        { dBUTTONS_PIN_ENTER, eBTN_EVENT_ENTER, 1, 1, 0 }
    },
    .gpioHandle = -1,
    .isRunning = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .queueHead = 0,
    .queueTail = 0
};

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static long Buttons_MsNow(void);
static void Buttons_PushQueue(btnEvent_t event);
static void *Buttons_ReaderThread(void *arg);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
* @brief Inicializa a leitura dos botoes e a thread de debounce.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Buttons_Init(void)
{
    buttons.gpioHandle = lgGpiochipOpen(dBUTTONS_GPIOCHIP);
    if(buttons.gpioHandle < 0)
    {
        fprintf(stderr, "botoes: lgGpiochipOpen falhou (%d)\n", buttons.gpioHandle);
        return -1;
    }

    for(int i = 0; i < dBUTTONS_COUNT; i++)
    {
        int result = lgGpioClaimInput(buttons.gpioHandle, LG_SET_PULL_UP, buttons.btnList[i].pin);
        if(result < 0)
        {
            fprintf(stderr, "botoes: GPIO %d indisponivel (%d)\n", buttons.btnList[i].pin, result);
            lgGpiochipClose(buttons.gpioHandle);
            buttons.gpioHandle = -1;
            return -1;
        }
    }

    buttons.isRunning = 1;
    if(pthread_create(&buttons.threadId, NULL, Buttons_ReaderThread, NULL) != 0)
    {
        buttons.isRunning = 0;
        lgGpiochipClose(buttons.gpioHandle);
        buttons.gpioHandle = -1;
        return -1;
    }

    return 0;
}

/**************************************
* @brief Retorna um evento pendente da fila de botoes.
* @param Nenhum.
* @retval Evento do botao (eBTN_EVENT_NONE se a fila estiver vazia).
***************************************/
btnEvent_t Buttons_Poll(void)
{
    btnEvent_t event = eBTN_EVENT_NONE;
    pthread_mutex_lock(&buttons.mutex);

    if(buttons.queueHead != buttons.queueTail)
    {
        event = buttons.eventQueue[buttons.queueHead];
        buttons.queueHead = (buttons.queueHead + 1) % dBUTTONS_QSIZE;
    }

    pthread_mutex_unlock(&buttons.mutex);
    return event;
}

/**************************************
* @brief Finaliza a thread dos botoes e libera a interface GPIO.
* @param Nenhum.
* @retval Nenhum.
***************************************/
void Buttons_Close(void)
{
    if(buttons.isRunning == 1)
    {
        buttons.isRunning = 0;
        pthread_join(buttons.threadId, NULL);
    }

    if(buttons.gpioHandle >= 0)
    {
        lgGpiochipClose(buttons.gpioHandle);
        buttons.gpioHandle = -1;
    }
}

/**************************************
* FUNCOES LOCAIS
***************************************/

/**************************************
* @brief Obtem o tempo monotonic atual em milissegundos.
* @param Nenhum.
* @retval Tempo atual em milissegundos.
***************************************/
static long Buttons_MsNow(void)
{
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    return (timeSpec.tv_sec * dBUTTONS_MS_PER_SEC) + (timeSpec.tv_nsec / dBUTTONS_NS_PER_MS);
}

/**************************************
* @brief Insere um evento na fila circular.
* @param event: evento do botao a ser adicionado.
* @retval Nenhum.
***************************************/
static void Buttons_PushQueue(btnEvent_t event)
{
    pthread_mutex_lock(&buttons.mutex);
    int nextTail = (buttons.queueTail + 1) % dBUTTONS_QSIZE;

    if(nextTail != buttons.queueHead)
    {
        buttons.eventQueue[buttons.queueTail] = event;
        buttons.queueTail = nextTail;
    }

    pthread_mutex_unlock(&buttons.mutex);
}

/**************************************
* @brief Thread leitora responsavel pela amostragem e debounce dos botoes.
* @param arg: argumento generico da thread.
* @retval Ponteiro nulo.
***************************************/
static void *Buttons_ReaderThread(void *arg)
{
    (void)arg;

    while(buttons.isRunning == 1)
    {
        long currentTimeMs = Buttons_MsNow();

        for(int i = 0; i < dBUTTONS_COUNT; i++)
        {
            buttonState_t *btnPtr = &buttons.btnList[i];
            int readValue = lgGpioRead(buttons.gpioHandle, btnPtr->pin);

            if(readValue < 0)
            {
                continue;
            }

            if(readValue != btnPtr->rawState)
            {
                btnPtr->rawState = readValue;
                btnPtr->lastChangeTimeMs = currentTimeMs;
            }
            else if((readValue != btnPtr->stableState) && ((currentTimeMs - btnPtr->lastChangeTimeMs) >= dBUTTONS_DEBOUNCE_MS))
            {
                btnPtr->stableState = readValue;
                if(readValue == 0)
                {
                    Buttons_PushQueue(btnPtr->event);
                }
            }
        }

        usleep(dBUTTONS_POLL_US);
    }

    return NULL;
}
/** @} DOXYGEN GROUP TAG END OF FILE */