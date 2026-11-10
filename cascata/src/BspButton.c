/*
 * @file    BspButton.c
 * @brief   Implementacao do debounce e da fila de eventos dos botoes.
 */

#include "BspButton.h"

#include "BspGpio.h"

#include <stddef.h>

/// Quantidade de botoes do projeto
#define dBUTTON_COUNT               2

/// Leituras consecutivas iguais para aceitar uma mudanca [amostras]
#define dBUTTON_DEBOUNCE_SAMPLES    3

/// Capacidade da fila de eventos [eventos]
#define dBUTTON_QUEUE_SIZE          8

/// Descricao fixa de cada botao (pino e evento gerado ao pressionar)
typedef struct
{
    bspGpioPin_t pin;
    bspButtonEvent_t event;
} buttonConfig_t;

/// Estado de debounce de cada botao
typedef struct
{
    bool stableLevel;       // Ultimo nivel aceito (true = solto)
    bool candidateLevel;    // Nivel que esta sendo validado
    uint8_t sampleCount;    // Leituras consecutivas do candidato
} buttonState_t;

/// Tabela de botoes (pull-up: pressionado = nivel baixo)
static const buttonConfig_t buttonConfig[dBUTTON_COUNT] =
{
    { eBSP_GPIO_BTN_SELECT, eBSP_BUTTON_EVENT_SELECT },
    { eBSP_GPIO_BTN_ENTER,  eBSP_BUTTON_EVENT_ENTER  }
};

/// Dados locais do modulo
static struct
{
    buttonState_t state[dBUTTON_COUNT];
    bspButtonEvent_t queue[dBUTTON_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} bspButton;

/*
 * @brief   Insere um evento no fim da fila. Se estiver cheia, descarta.
 * @param   event - Evento a ser enfileirado
 * @retval  void
 */
static void BspButton_Push(bspButtonEvent_t event)
{
    if(bspButton.count < dBUTTON_QUEUE_SIZE)
    {
        bspButton.queue[bspButton.head] = event;
        bspButton.head = (uint8_t)((bspButton.head + 1U) % dBUTTON_QUEUE_SIZE);
        bspButton.count++;
    }
}

/*
 * @brief   Processa uma amostra de um botao (debounce e deteccao de clique).
 * @param   index - Indice do botao na tabela buttonConfig
 * @param   level - Nivel lido agora (true = solto, false = pressionado)
 * @retval  void
 */
static void BspButton_Process(uint8_t index, bool level)
{
    buttonState_t *state = &bspButton.state[index];

    if(level != state->candidateLevel)
    {
        state->candidateLevel = level;
        state->sampleCount = 0;
        return;
    }

    if(state->sampleCount < dBUTTON_DEBOUNCE_SAMPLES)
    {
        state->sampleCount++;
    }

    if((state->sampleCount == dBUTTON_DEBOUNCE_SAMPLES)
       && (state->candidateLevel != state->stableLevel))
    {
        state->stableLevel = state->candidateLevel;

        // Clique registrado apenas na borda de descida (botao pressionado)
        if(state->stableLevel == false)
        {
            BspButton_Push(buttonConfig[index].event);
        }
    }
}

bool BspButton_Init(void)
{
    uint8_t index;
    bool level = true;

    bspButton.head = 0;
    bspButton.tail = 0;
    bspButton.count = 0;

    for(index = 0; index < dBUTTON_COUNT; index++)
    {
        if(BspGpio_Read(buttonConfig[index].pin, &level) == false)
        {
            return false;
        }

        bspButton.state[index].stableLevel = level;
        bspButton.state[index].candidateLevel = level;
        bspButton.state[index].sampleCount = dBUTTON_DEBOUNCE_SAMPLES;
    }

    return true;
}

void BspButton_Poll(void)
{
    uint8_t index;
    bool level = true;

    for(index = 0; index < dBUTTON_COUNT; index++)
    {
        if(BspGpio_Read(buttonConfig[index].pin, &level) == true)
        {
            BspButton_Process(index, level);
        }
    }
}

bspButtonEvent_t BspButton_GetEvent(void)
{
    bspButtonEvent_t event = eBSP_BUTTON_EVENT_NONE;

    if(bspButton.count > 0)
    {
        event = bspButton.queue[bspButton.tail];
        bspButton.tail = (uint8_t)((bspButton.tail + 1U) % dBUTTON_QUEUE_SIZE);
        bspButton.count--;
    }

    return event;
}