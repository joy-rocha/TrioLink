/**************************************
* @file Buttons.c
* @addtogroup Buttons
* @brief Modulo de leitura e debounce de botoes via GPIO.
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
/// Chip GPIO padrao
#define dGPIO_CHIP         0
/// Tempo de debounce em milissegundos
#define dDEBOUNCE_MS      30
/// Tempo de varredura em microsegundos
#define dPOLL_US          2000
/// Tamanho da fila circular de eventos
#define dQSIZE            16
/// Quantidade de botoes suportados
#define dNUMBER_OF_BUTTONS 2
/// Nivel estavel liberado
#define dSTABLE_RELEASED  1
/// Nivel estavel pressionado
#define dSTABLE_PRESSED   0
/// Fator de conversao de segundos para ms
#define dMS_PER_SEC       1000L
/// Fator de conversao de nanosegundos para ms
#define dNS_PER_MS        1000000L

/**************************************
* CONSTANTES
***************************************/

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
***************************************/
/// Estrutura para controle individual de cada botao
typedef struct
{
    int pin;
    btnEvent_t event;
    int stable;
    int raw;
    long timeStamp;
} btnState_t;

/// Variaveis internas do modulo Buttons
static struct
{
    btnState_t btnList[dNUMBER_OF_BUTTONS];
    int gpioHandle;
    pthread_t thread;
    volatile bool isRunning;
    pthread_mutex_t mutex;
    btnEvent_t queue[dQSIZE];
    int queueHead;
    int queueTail;
} buttons = {
    .btnList = {
        { dBUTTONS_PIN_BTN_NAV, eBTN_EVENT_NAV, dSTABLE_RELEASED, dSTABLE_RELEASED, 0 },
        { dBUTTONS_PIN_BTN_ENTER, eBTN_EVENT_ENTER, dSTABLE_RELEASED, dSTABLE_RELEASED, 0 }
    },
    .gpioHandle = -1,
    .isRunning = false,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .queueHead = 0,
    .queueTail = 0
};

/**************************************
* PROTOTIPOS LOCAIS
***************************************/
static long Buttons_MsNow(void);
static void Buttons_Push(btnEvent_t event);
static void *Buttons_ReaderThread(void *arg);

/**************************************
* FUNCOES PUBLICAS
***************************************/

/**************************************
/** @brief Inicializa o driver de botoes e a thread de leitura.
* @param Nenhum.
* @retval 0 em caso de sucesso, -1 em caso de erro.
***************************************/
int Buttons_Init(void)
{
    buttons.gpioHandle = lgGpiochipOpen(dGPIO_CHIP);
    if(buttons.gpioHandle < 0)
    {
        fprintf(stderr, "botoes: lgGpiochipOpen falhou (%d)\n", buttons.gpioHandle);
        return -1;
    }

    for(int i = 0; i < dNUMBER_OF_BUTTONS; i++)
    {
        int result = lgGpioClaimInput(buttons.gpioHandle, LG_SET_PULL_UP, buttons.btnList[i].pin);
        if(result < 0)
        {
            fprintf(stderr, "botoes: GPIO %d indisponivel (%d). Esta em uso pelo LCD?\n", buttons.btnList[i].pin, result);
            lgGpiochipClose(buttons.gpioHandle);
            buttons.gpioHandle = -1;
            return -1;
        }
    }

    buttons.isRunning = true;
    if(pthread_create(&buttons.thread, NULL, Buttons_ReaderThread, NULL) != 0)
    {
        buttons.isRunning = false;
        lgGpiochipClose(buttons.gpioHandle);
        buttons.gpioHandle = -1;
        return -1;
    }

    return 0;
}

/**************************************
/** @brief Retorna um evento pendente da fila de botoes.
* @param Nenhum.
* @retval Evento do botao (eBTN_EVENT_NONE se nao houver evento).
***************************************/
btnEvent_t Buttons_Poll(void)
{
    btnEvent_t event = eBTN_EVENT_NONE;

    pthread_mutex_lock(&buttons.mutex);
    if(buttons.queueHead != buttons.queueTail)
    {
        event = buttons.queue[buttons.queueHead];
        buttons.queueHead = (buttons.queueHead + 1) % dQSIZE;
    }
    pthread_mutex_unlock(&buttons.mutex);

    return event;
}

/**************************************
/** @brief Encerra a thread de leitura e libera os recursos GPIO.
* @param Nenhum.
* @retval Nenhum.
***************************************/
void Buttons_Close(void)
{
    if(buttons.isRunning == true)
    {
        buttons.isRunning = false;
        pthread_join(buttons.thread, NULL);
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
/** @brief Obtem o tempo atual em milissegundos a partir do clock monotonico.
* @param Nenhum.
* @retval Tempo em milissegundos.
***************************************/
static long Buttons_MsNow(void)
{
    struct timespec timeSpec;
    clock_gettime(CLOCK_MONOTONIC, &timeSpec);
    return (timeSpec.tv_sec * dMS_PER_SEC) + (timeSpec.tv_nsec / dNS_PER_MS);
}

/**************************************
/** @brief Adiciona um evento na fila circular.
* @param event: evento a ser adicionado na fila.
* @retval Nenhum.
***************************************/
static void Buttons_Push(btnEvent_t event)
{
    pthread_mutex_lock(&buttons.mutex);
    int nextTail = (buttons.queueTail + 1) % dQSIZE;
    if(nextTail != buttons.queueHead)
    {
        buttons.queue[buttons.queueTail] = event;
        buttons.queueTail = nextTail;
    }
    pthread_mutex_unlock(&buttons.mutex);
}

/**************************************
/** @brief Thread responsavel pela varredura continua dos botoes com debounce.
* @param arg: ponteiro generico de argumentos.
* @retval Ponteiro nulo.
***************************************/
static void *Buttons_ReaderThread(void *arg)
{
    (void)arg;

    while(buttons.isRunning == true)
    {
        long currentTime = Buttons_MsNow();

        for(int i = 0; i < dNUMBER_OF_BUTTONS; i++)
        {
            btnState_t *btnPtr = &buttons.btnList[i];
            int readVal = lgGpioRead(buttons.gpioHandle, btnPtr->pin);

            if(readVal < 0)
            {
                continue;
            }

            if(readVal != btnPtr->raw)
            {
                btnPtr->raw = readVal;
                btnPtr->timeStamp = currentTime;
            }
            else if((readVal != btnPtr->stable) && ((currentTime - btnPtr->timeStamp) >= dDEBOUNCE_MS))
            {
                btnPtr->stable = readVal;
                if(readVal == dSTABLE_PRESSED)
                {
                    Buttons_Push(btnPtr->event);
                }
            }
        }

        usleep(dPOLL_US);
    }

    return NULL;
}
/** @} DOXYGEN GROUP TAG END OF FILE */