/*
 * @file    BspScreen.h
 * @brief   Interface grafica (GTK4 + Cairo) e driver do display ILI9341.
 */

#ifndef BSP_SCREEN_H
#define BSP_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

/*
 * @brief   Inicializa o display, abre a janela GTK e executa o loop de
 *          eventos (botoes, atualizacao dos dados e desenho) ate o usuario
 *          fechar a janela ou receber SIGINT/SIGTERM.
 * @param   argc - Quantidade de argumentos da linha de comando
 * @param   argv - Argumentos da linha de comando
 * @retval  0 - Encerrou normalmente
 * @retval  1 - Falha ao criar os recursos graficos
 * @warning BspGpio_Init, BspButton_Init e Fusion_Init devem ter sido
 *          executadas antes.
 */
int BspScreen_Run(int argc, char **argv);

#endif /* BSP_SCREEN_H */