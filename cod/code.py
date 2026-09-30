import os
os.environ["SDL_VIDEODRIVER"] = "dummy"

import sys
import pygame
import json
import subprocess
import time
import threading

# Utiliza o dispositivo virtual 'dummy' do Luma para renderização direta em memória
from luma.core.device import dummy

# Tenta carregar a biblioteca de hardware da RPi 5
try:
    import lgpio
    IS_RPI = True
except ImportError:
    IS_RPI = False

# Importa as funções de ecrã do seu ficheiro Screnns.py
from Screnns import (
    Display_MainScreen,
    Display_ScreenMpu,
    Display_ScrennBpm,
    Display_ScrennOFF,
    Display_ScrennON
)

# Inicializa o dispositivo Luma em memória (320x240 pixels em RGB)
device = dummy(width=320, height=240, mode="RGB")

# Inicializa o Pygame
pygame.init()
pygame.display.set_mode((320, 240))

# ==============================================================================
# CONFIGURAÇÃO DE HARDWARE CONFORME TABELA MAR2406 (BCM RPi 5)
# ==============================================================================
if IS_RPI:
    # Pinos de Controle (BCM)
    PIN_RST = 17  # Pino físico 11
    PIN_CS  = 27  # Pino físico 13
    PIN_RS  = 22  # Pino físico 15 (Comando/Dados)
    PIN_WR  = 23  # Pino físico 16
    PIN_RD  = 24  # Pino físico 18

    # Pinos de Dados D0 a D7 (BCM)
    D_PINS = [5, 6, 12, 13, 16, 19, 20, 21]

    try:
        gpio_chip = lgpio.gpiochip_open(4)
    except Exception:
        try:
            gpio_chip = lgpio.gpiochip_open(0)
        except Exception:
            gpio_chip = None

    if gpio_chip is not None:
        for p in [PIN_RD, PIN_WR, PIN_RS, PIN_RST, PIN_CS] + D_PINS:
            lgpio.gpio_claim_output(gpio_chip, p, 1)

    def write_byte(val):
        val = int(val)
        for i in range(8):
            bit_val = (val >> i) & 1
            lgpio.gpio_write(gpio_chip, D_PINS[i], bit_val)
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
        if gpio_chip is None:
            return
        lgpio.gpio_write(gpio_chip, PIN_RD, 1)
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

    def enviar_para_display_fisico(image):
        if image is None or gpio_chip is None:
            return

        # Define a área de escrita no ILI9341 (320x240)
        write_cmd(0x2A) # Colunas (0 a 319)
        write_data(0); write_data(0); write_data(1); write_data(63)
        write_cmd(0x2B) # Linhas (0 a 239)
        write_data(0); write_data(0); write_data(0); write_data(239)
        write_cmd(0x2C) # Escrita na memória RAM

        lgpio.gpio_write(gpio_chip, PIN_RS, 1)
        lgpio.gpio_write(gpio_chip, PIN_CS, 0)

        # Converte a imagem Pillow para bytes RGB e envia como RGB565
        img_rgb = image.convert("RGB")
        raw_rgb = img_rgb.tobytes()

        for i in range(0, len(raw_rgb), 3):
            r = raw_rgb[i]
            g = raw_rgb[i+1]
            b = raw_rgb[i+2]

            high = (r & 0xF8) | ((g & 0xE0) >> 5)
            low = ((g & 0x1C) << 3) | (b >> 3)

            write_byte(high)
            write_byte(low)

        lgpio.gpio_write(gpio_chip, PIN_CS, 1)

    inicializar_ili9341()

# ==============================================================================
# LÓGICA DAS TELAS E NAVEGAÇÃO
# ==============================================================================
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

# ==============================================================================
# LEITOR DE TOUCH DIRETO DO SISTEMA (EVDEV)
# ==============================================================================
def iniciar_thread_touch():
    try:
        import evdev
        from evdev import InputDevice, ecodes
    except ImportError:
        print("[AVISO] Pacote 'evdev' não instalado. Instale com 'pip install evdev' para melhor suporte ao touch.")
        return

    def loop_leitura_touch():
        touch_device = None
        try:
            for path in evdev.list_devices():
                dev = evdev.InputDevice(path)
                caps = dev.capabilities()
                if ecodes.EV_ABS in caps:
                    touch_device = dev
                    break
        except Exception:
            return

        if not touch_device:
            return

        print(f"[TOUCH] Dispositivo de toque encontrado: {touch_device.name}")

        abs_x = touch_device.absinfo(ecodes.ABS_X) if ecodes.ABS_X in touch_device.capabilities().get(ecodes.EV_ABS, []) else None
        abs_y = touch_device.absinfo(ecodes.ABS_Y) if ecodes.ABS_Y in touch_device.capabilities().get(ecodes.EV_ABS, []) else None

        min_x = abs_x.min if abs_x else 0
        max_x = abs_x.max if abs_x else 320
        min_y = abs_y.min if abs_y else 0
        max_y = abs_y.max if abs_y else 240

        curr_x, curr_y = 0, 0
        pressionado = False

        try:
            for event in touch_device.read_loop():
                if event.type == ecodes.EV_ABS:
                    if event.code in (ecodes.ABS_X, ecodes.ABS_MT_POSITION_X):
                        curr_x = int((event.value - min_x) * 320 / max(1, (max_x - min_x)))
                    elif event.code in (ecodes.ABS_Y, ecodes.ABS_MT_POSITION_Y):
                        curr_y = int((event.value - min_y) * 240 / max(1, (max_y - min_y)))
                elif event.type == ecodes.EV_KEY:
                    if event.code in (ecodes.BTN_TOUCH, ecodes.BTN_MOUSE):
                        if event.value == 1:
                            pressionado = True
                        elif event.value == 0 and pressionado:
                            pressionado = False
                            cx = max(0, min(319, curr_x))
                            cy = max(0, min(239, curr_y))
                            Display_TrateClick(cx, cy)
        except Exception as e:
            print(f"[TOUCH Error] {e}")

    t = threading.Thread(target=loop_leitura_touch, daemon=True)
    t.start()

# Inicia a escuta do touch físico em segundo plano
if IS_RPI:
    iniciar_thread_touch()

# ==============================================================================
# COMUNICAÇÃO COM O PROGRAMA EM C
# ==============================================================================
def obter_dados_do_c():
    try:
        resultado = subprocess.run(["./meu_programa"], capture_output=True, text=True, check=True)
        return json.loads(resultado.stdout)
    except Exception:
        return {}

# ==============================================================================
# LOOP PRINCIPAL
# ==============================================================================
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

        # Envia a imagem renderizada para o display físico
        if IS_RPI and hasattr(device, 'image'):
            enviar_para_display_fisico(device.image)

        # Processa eventos do Pygame (mouse/touch no ambiente gráfico ou simulação)
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                sys.exit()
            elif event.type == pygame.MOUSEBUTTONDOWN:
                x_mouse = int(event.pos[0])
                y_mouse = int(event.pos[1])
                Display_TrateClick(x_mouse, y_mouse)
            elif event.type == pygame.FINGERDOWN:
                x_touch = int(event.x * 320)
                y_touch = int(event.y * 240)
                Display_TrateClick(x_touch, y_touch)

        time.sleep(0.03)