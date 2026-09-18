# Display.py

import os
os.environ["SDL_VIDEODRIVER"] = "dummy"  # Permite renderizar em memória sem monitor HDMI

import sys
import pygame
import json
import subprocess
import time

# IMPORTAÇÃO DO EMULADOR LUMA PARA CRIAR O 'device'
from luma.emulator.device import pygame as PygameDevice

# Tenta carregar a biblioteca de hardware da RPi 5
try:
    import lgpio
    import numpy as np
    IS_RPI = True
except ImportError:
    IS_RPI = False

try:
    import evdev
    from evdev import ecodes
    HAS_EVDEV = True
except ImportError:
    HAS_EVDEV = False

# Importa as funções de ecrã do seu ficheiro Screnns.py
from Screnns import (
    Display_MainScreen,
    Display_ScreenMpu,
    Display_ScrennBpm,
    Display_ScrennOFF,
    Display_ScrennON
)

# ==============================================================================
# INITIALIZAÇÃO DO DISPOSITIVO VIRTUAL (LUMA)
# ==============================================================================
# Cria o objeto 'device' que o Screnns.py exige para renderizar com canvas(device)
device = PygameDevice(width=320, height=240)

# ==============================================================================
# CONFIGURAÇÃO DE HARDWARE (EXECUTADO APENAS NA RASPBERRY PI 5)
# ==============================================================================
if IS_RPI:
    PIN_RD, PIN_WR, PIN_RS, PIN_RST, PIN_CS = 17, 27, 24, 25, 8
    D_PINS = [2, 3, 4, 5, 6, 7, 9, 10]  # D0 a D7 conforme a sua tabela

    try:
        gpio_chip = lgpio.gpiochip_open(4)
    except Exception:
        gpio_chip = lgpio.gpiochip_open(0)

    for p in [PIN_RD, PIN_WR, PIN_RS, PIN_RST, PIN_CS] + D_PINS:
        lgpio.gpio_claim_output(gpio_chip, p, 1)

    def write_byte(val):
        for i in range(8):
            lgpio.gpio_write(gpio_chip, D_PINS[i], (val >> i) & 1)
        lgpio.gpio_write(gpio_chip, PIN_WR, 0)
        lgpio.gpio_write(gpio_chip, PIN_WR, 1)

    def write_cmd(cmd):
        lgpio.gpio_write(gpio_chip, PIN_RS, 0)
        lgpio.gpio_write(gpio_chip, PIN_CS, 0)
        write_byte(cmd)
        lgpio.gpio_write(gpio_chip, PIN_CS, 1)

    def write_data(data):
        lgpio.gpio_write(gpio_chip, PIN_RS, 1)
        lgpio.gpio_write(gpio_chip, PIN_CS, 0)
        write_byte(data)
        lgpio.gpio_write(gpio_chip, PIN_CS, 1)

    def inicializar_ili9341():
        lgpio.gpio_write(gpio_chip, PIN_RD, 1) # Pino de leitura desativado (HIGH)
        lgpio.gpio_write(gpio_chip, PIN_RST, 0)
        time.sleep(0.05)
        lgpio.gpio_write(gpio_chip, PIN_RST, 1)
        time.sleep(0.12)
        
        write_cmd(0x01) # Software Reset
        time.sleep(0.01)
        
        write_cmd(0x3A) # Pixel Format
        write_data(0x55) # 16-bit RGB565
        
        write_cmd(0x36) # Memory Access Control (Orientação Paisagem)
        write_data(0x28)
        
        write_cmd(0x11) # Sleep Out
        time.sleep(0.12)
        
        write_cmd(0x29) # Display ON

    def enviar_para_display_fisico(surface):
        # Define janela total 320x240
        write_cmd(0x2A) # Colunas (0 a 319)
        write_data(0); write_data(0); write_data(1); write_data(63)
        write_cmd(0x2B) # Linhas (0 a 239)
        write_data(0); write_data(0); write_data(0); write_data(239)
        write_cmd(0x2C) # Escrita na memória RAM

        pixels = pygame.surfarray.pixels3d(surface)
        lgpio.gpio_write(gpio_chip, PIN_RS, 1)
        lgpio.gpio_write(gpio_chip, PIN_CS, 0)

        # Transfere matriz de píxeis para formato RGB565
        for y in range(240):
            for x in range(320):
                r, g, b = pixels[x, y]
                cor = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
                write_byte((cor >> 8) & 0xFF)
                write_byte(cor & 0xFF)

        lgpio.gpio_write(gpio_chip, PIN_CS, 1)

    inicializar_ili9341()


tela_atual = "MAIN"

def Display_TrateClick(x, y):
    global tela_atual
    if tela_atual == "MAIN":
        if 10 <= x <= 155 and 40 <= y <= 160: tela_atual = "BMP"
        elif 165 <= x <= 310 and 40 <= y <= 160: tela_atual = "MPU"
        elif 10 <= x <= 310 and 180 <= y <= 230: tela_atual = "OFF"
    elif tela_atual in ["BMP", "MPU", "OFF"]:
        if 150 <= x <= 320 and 0 <= y <= 40: tela_atual = "MAIN"
        elif tela_atual == "OFF":
            if 30 <= x <= 145 and 165 <= y <= 210:
                print("Desligando o sistema...")
                tela_atual = "ON"
            elif 175 <= x <= 290 and 165 <= y <= 210: tela_atual = "MAIN"
    elif tela_atual == "ON":
        if 60 <= x <= 260 and 95 <= y <= 145: tela_atual = "MAIN"

def obter_dados_do_c():
    try:
        resultado = subprocess.run(["./meu_programa"], capture_output=True, text=True, check=True)
        return json.loads(resultado.stdout)
    except Exception:
        return {}

if __name__ == "__main__":
    while True:
        dados = obter_dados_do_c()
        dados_bmp = dados.get("bmp", {})
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

        # Renderização das telas em memória com o Luma
        if tela_atual == "MAIN": Display_MainScreen(device, SensorMPU, SensorBPM)
        elif tela_atual == "BMP": Display_ScrennBpm(device, SensorBPM)
        elif tela_atual == "MPU": Display_ScreenMpu(device, SensorMPU)
        elif tela_atual == "OFF": Display_ScrennOFF(device)
        elif tela_atual == "ON": Display_ScrennON(device)

        # Se estiver na RPi 5, envia a imagem gerada para o ecrã físico
        if IS_RPI:
            surface = pygame.display.get_surface()
            if surface:
                enviar_para_display_fisico(surface)

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                sys.exit()
            elif event.type == pygame.MOUSEBUTTONDOWN:
                win_w, win_h = pygame.display.get_surface().get_size()
                x_mouse = int(event.pos[0] * (320.0 / win_w))
                y_mouse = int(event.pos[1] * (240.0 / win_h))
                Display_TrateClick(x_mouse, y_mouse)

        time.sleep(0.03)  # Pausa para estabilidade e baixo consumo de CPU