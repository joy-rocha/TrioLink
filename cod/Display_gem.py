import os
os.environ["SDL_VIDEODRIVER"] = "dummy"

import sys
import pygame
import json
import subprocess
import time
import threading
import ctypes

from luma.core.device import dummy

try:
    import lgpio
    IS_RPI = True
except ImportError:
    IS_RPI = False

from tela_modelo import (
    Display_MainScreen,
    Display_ScreenMpu,
    Display_ScrennBpm,
    Display_ScrennOFF,
    Display_ScrennON
)

device = dummy(width=320, height=240, mode="RGB")

pygame.init()
pygame.display.set_mode((320, 240))

driver_c = None
ultima_imagem_bytes = None

if IS_RPI:
    c_lib_path = os.path.join(os.path.dirname(__file__), "display_driver.so")
    if os.path.exists(c_lib_path):
        try:
            driver_c = ctypes.CDLL(c_lib_path)
            print("[INFO] Inicializando driver de hardware em C...", flush=True)
            driver_c.init_display()
            print("[SUCESSO] Driver C carregado!", flush=True)
        except Exception as e:
            print(f"[ERRO] Falha ao carregar driver C: {e}", flush=True)
    else:
        print("[AVISO] display_driver.so nao encontrado.", flush=True)

def enviar_para_display_fisico(image):
    global ultima_imagem_bytes
    if image is None or driver_c is None:
        return

    img_rgb = image.convert("RGB")
    raw_rgb = img_rgb.tobytes()

    # Otimização: Só transmite se houver mudança nos pixels
    if raw_rgb == ultima_imagem_bytes:
        return

    ultima_imagem_bytes = raw_rgb
    driver_c.send_frame(raw_rgb)

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
                print("[INFO] Desligando o sistema...", flush=True)
                tela_atual = "ON"
            elif 175 <= x <= 290 and 165 <= y <= 210: tela_atual = "MAIN"
    elif tela_atual == "ON":
        if 60 <= x <= 260 and 95 <= y <= 145: tela_atual = "MAIN"

def iniciar_thread_touch():
    try:
        import evdev
        from evdev import ecodes
    except ImportError:
        return

    def loop_leitura_touch():
        touch_device = None
        try:
            for path in evdev.list_devices():
                dev = evdev.InputDevice(path)
                caps = dev.capabilities()
                if ecodes.EV_ABS in caps or ecodes.EV_KEY in caps:
                    touch_device = dev
                    break
        except Exception:
            return

        if not touch_device:
            return

        curr_x, curr_y = 0, 0
        pressionado = False

        try:
            for event in touch_device.read_loop():
                if event.type == ecodes.EV_ABS:
                    if event.code in (ecodes.ABS_X, ecodes.ABS_MT_POSITION_X):
                        curr_x = int(event.value * 320 / 4095) if event.value > 320 else event.value
                    elif event.code in (ecodes.ABS_Y, ecodes.ABS_MT_POSITION_Y):
                        curr_y = int(event.value * 240 / 4095) if event.value > 240 else event.value
                elif event.type == ecodes.EV_KEY:
                    if event.code in (ecodes.BTN_TOUCH, ecodes.BTN_MOUSE):
                        if event.value == 1:
                            pressionado = True
                        elif event.value == 0 and pressionado:
                            pressionado = False
                            Display_TrateClick(curr_x, curr_y)
        except Exception:
            pass

    t = threading.Thread(target=loop_leitura_touch, daemon=True)
    t.start()

if IS_RPI:
    iniciar_thread_touch()

def obter_dados_do_c():
    meu_prog = os.path.join(os.path.dirname(__file__), "meu_programa")
    if not os.path.exists(meu_prog):
        # Dados simulados para o teste visual
        return {
            "mpu": {"status": "online", "direcao": 45, "velocidade": 12, "estadoMPU": "ok"},
            "bmp": {"status": "online", "pressao": 1013, "temperatura": 25, "altitude": 100, "estadoBPM": "ok"}
        }
    try:
        resultado = subprocess.run([meu_prog], capture_output=True, text=True, check=True)
        return json.loads(resultado.stdout)
    except Exception:
        return {}

if __name__ == "__main__":
    print("[INFO] Iniciando loop de interface...", flush=True)
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

        if tela_atual == "MAIN": Display_MainScreen(device, SensorMPU, SensorBPM)
        elif tela_atual == "BMP": Display_ScrennBpm(device, SensorBPM)
        elif tela_atual == "MPU": Display_ScreenMpu(device, SensorMPU)
        elif tela_atual == "OFF": Display_ScrennOFF(device)
        elif tela_atual == "ON": Display_ScrennON(device)

        if IS_RPI and hasattr(device, 'image'):
            enviar_para_display_fisico(device.image)

        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                sys.exit()
            elif event.type == pygame.MOUSEBUTTONDOWN:
                Display_TrateClick(event.pos[0], event.pos[1])

        time.sleep(0.03)