import sys
import pygame
from luma.emulator.device import pygame as pygame_device

import json
import subprocess

# Tenta importar evdev (presente no Linux/Raspberry Pi)
try:
    import evdev
    from evdev import ecodes
    HAS_EVDEV = True
except ImportError:
    HAS_EVDEV = False

# Importa todas as funções de Screnns.py
from Screnns import (
    Display_MainScreen,
    Display_ScreenMpu,
    Display_ScrennBpm,
    Display_ScrennOFF,
)

# Inicialização do Display
device = pygame_device(width=320, height=240)

# Estado global do sistema
tela_atual = "MAIN"  # Opções: "MAIN", "BMP", "MPU", "OFF"


def Display_TrateClick(x, y):
    global tela_atual

    if tela_atual == "MAIN":
        if 10 <= x <= 155 and 40 <= y <= 160:
            tela_atual = "BMP"
        elif 165 <= x <= 310 and 40 <= y <= 160:
            tela_atual = "MPU"
        elif 10 <= x <= 310 and 180 <= y <= 230:
            tela_atual = "OFF"

    elif tela_atual in ["BMP", "MPU", "OFF"]:
        if 150 <= x <= 320 and 0 <= y <= 40:
            tela_atual = "MAIN"
        elif tela_atual == "OFF":
            if 30 <= x <= 145 and 165 <= y <= 210:
                print("Encerrando o sistema...")
                sys.exit()
            elif 175 <= x <= 290 and 165 <= y <= 210:
                tela_atual = "MAIN"


def obter_dados_do_c():
    try:
        # Executa o programa C e captura o JSON impresso no stdout
        resultado = subprocess.run(["./meu_programa"], capture_output=True, text=True, check=True)
        return json.loads(resultado.stdout)
    except Exception as e:
        print(f"Erro ao ler dados do C: {e}")
        return {}

# DADOS DE TESTE E CÓDIGO PRINCIPAL
if __name__ == "__main__":

    dados = obter_dados_do_c()# Captura os dados do C
    dados_bmp = dados.get("bmp", {})# Separa os dados de cada sensor
    dados_mpu = dados.get("mpu", {})

    SensorMPU = {
        "status": str(dados_mpu.get("status", "online")).upper(),
        "direcao": f"{dados_mpu.get('direcao', 0)} deg",
        "velocidade": f"{dados_mpu.get('velocidade', 0)} m/s",
        "estado": str(dados_mpu.get("estadoMPU", "normal")).upper()
    }

    SensorBPM = {
        "status": str(dados_bmp.get("status", "offline")).upper(),
        "pressao": f"{dados_bmp.get('pressao', 0)} hPa",
        "temperatura": f"{dados_bmp.get('temperatura', 0)}° C",
        "altitude": f"{dados_bmp.get('altitude', 0)} m",
        "estado": str(dados_bmp.get("estadoBPM", "normal")).upper()
    }

    while True:
        if tela_atual == "MAIN":
            Display_MainScreen(device, SensorMPU, SensorBPM)
        elif tela_atual == "BMP":
            Display_ScrennBpm(device, SensorBPM)
        elif tela_atual == "MPU":
            Display_ScreenMpu(device, SensorMPU)
        elif tela_atual == "OFF":
            Display_ScrennOFF(device)

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                sys.exit()
            elif event.type == pygame.MOUSEBUTTONDOWN:
                win_w, win_h = pygame.display.get_surface().get_size()
                x_mouse = int(event.pos[0] * (320.0 / win_w))
                y_mouse = int(event.pos[1] * (240.0 / win_h))
                Display_TrateClick(x_mouse, y_mouse)