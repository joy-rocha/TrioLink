/**************************************
* @file SimpleMovingAvg.c
* @addtogroup SimpleMovingAvg
* @brief Media movel simples com janela circular de tamanho fixo.
* @author Seu Nome
* @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - AssertTypes.
* \n <b>Observacoes:</b>
* - Nao utiliza alocacao dinamica: a janela e um array cujo tamanho e definido
*   por dSIMPLE_MOVING_AVG_WINDOW_SIZE;
* - Toda instancia deve ser inicializada com SimpleMovingAvg_Init antes do uso;
* - A soma e mantida de forma incremental, entao o erro de arredondamento do
*   float pode se acumular em execucoes muito longas;
* - Nao e reentrante: se a mesma instancia for usada em interrupcao e no laco
*   principal, o acesso deve ser protegido pela aplicacao.
* Changelog
* @version <b>1.0.0 - 18/09/2026</b> \n Seu Nome \n Primeira versao.
* @copyright https://git.assert.ifpb.edu.br/Assert/Simple-Moving-Avg
***************************************/

/**************************************
* INCLUDES
***************************************/
#include "SimpleMovingAvg.h"

/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
***************************************/
/// Menor tamanho valido da janela
#define dWINDOW_SIZE_MIN 1
/// Maior tamanho valido da janela (indice e contador da janela sao u8)
#define dWINDOW_SIZE_MAX 255

// Verificacao de integridade das configuracoes
#if ((dSIMPLE_MOVING_AVG_WINDOW_SIZE < dWINDOW_SIZE_MIN) || \
     (dSIMPLE_MOVING_AVG_WINDOW_SIZE > dWINDOW_SIZE_MAX))
    #error "dSIMPLE_MOVING_AVG_WINDOW_SIZE fora da faixa permitida (1 a 255)."
#endif

/**************************************
* FUNCOES PUBLICAS
***************************************/
/**************************************
/** @brief Zera uma instancia de media movel.
* @param avg: ponteiro da instancia a ser inicializada.
* @retval Nenhum.
***************************************/
void SimpleMovingAvg_Init(simpleMovingAvg_t *avg)
{
    u8 i;

    for(i = 0; i < dSIMPLE_MOVING_AVG_WINDOW_SIZE; i++)
    {
        avg->buffer[i] = 0.0f;
    }

    avg->sum = 0.0f;
    avg->index = 0;
    avg->count = 0;
}

/**************************************
/** @brief Insere uma nova amostra na janela, descartando a mais antiga
*         quando a janela ja esta cheia.
* @param avg: ponteiro da instancia de media movel.
* @param sample: nova amostra a ser inserida.
* @retval Nenhum.
***************************************/
void SimpleMovingAvg_NewSample(simpleMovingAvg_t *avg, float sample)
{
    // Remove da soma a amostra mais antiga (zero enquanto a janela nao enche)
    avg->sum -= avg->buffer[avg->index];
    avg->buffer[avg->index] = sample;
    avg->sum += sample;

    avg->index = (u8)((avg->index + 1) % dSIMPLE_MOVING_AVG_WINDOW_SIZE);

    if(avg->count < dSIMPLE_MOVING_AVG_WINDOW_SIZE)
    {
        avg->count++;
    }
}

/**************************************
/** @brief Retorna a media das amostras atualmente na janela.
* @param avg: ponteiro da instancia de media movel.
* @retval Media das amostras validas; zero se ainda nao houver amostras.
* @details Antes de a janela encher, a media e feita sobre a quantidade real
*          de amostras recebidas, e nao sobre o tamanho total da janela.
***************************************/
float SimpleMovingAvg_GetValue(const simpleMovingAvg_t *avg)
{
    float value = 0.0f;

    if(avg->count > 0)
    {
        value = (avg->sum / (float)avg->count);
    }

    return value;
}

/**************************************
/** @brief Informa se a janela ja recebeu amostras suficientes para enche-la.
* @param avg: ponteiro da instancia de media movel.
* @retval true se a janela esta cheia; false caso contrario.
***************************************/
bool SimpleMovingAvg_IsBufferFull(const simpleMovingAvg_t *avg)
{
    bool isFull = false;

    if(avg->count == dSIMPLE_MOVING_AVG_WINDOW_SIZE)
    {
        isFull = true;
    }

    return isFull;
}

/** @} DOXYGEN GROUP TAG END OF FILE */
