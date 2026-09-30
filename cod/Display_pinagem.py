#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Display_pinagem.py

Display modular:
- NÃO altera Screnns.py.
- Usa as funções de desenho existentes em Screnns.py.
- ILI9341/MAR2406 em paralelo 8 bits.
- lgpio para LCD.
- evdev para touch, quando o Linux disponibilizar um event touchscreen.
- leitura de ./meu_programa permanece igual ao projeto anterior.

PINAGEM BCM:
RST = 17
CS  = 27
RS  = 22
WR  = 23
RD  = 24
D0  = 5
D1  = 6
D2  = 12
D3  = 13
D4  = 16
D5  = 19
D6  = 20
D7  = 21

IMPORTANTE:
No Raspberry Pi 5, os GPIOs do header são acessados pelo gpiochip0
nas instalações atuais do Raspberry Pi OS. Por isso gpiochip0 é usado
explicitamente aqui.
"""

import json
import os
import select
import signal
import subprocess
import sys
import time
from collections import namedtuple

import lgpio
from luma.core.device import dummy

try:
    from evdev import InputDevice, list_devices, ecodes
except ImportError:
    InputDevice = None
    list_devices = lambda: []
    ecodes = None


# ============================================================
# CONFIGURAÇÃO
# ============================================================

WIDTH = 320
HEIGHT = 240

PIN_RST = 17
PIN_CS = 27
PIN_RS = 22
PIN_WR = 23
PIN_RD = 24

D_PINS = [5, 6, 12, 13, 16, 19, 20, 21]

# Raspberry Pi 5 / header GPIO
GPIO_CHIP = 0

# 320x240 em paisagem.
MADCTL = 0x48

C_PROGRAM = "./meu_programa"
DATA_INTERVAL = 0.30

TOUCH_DEVICE = os.environ.get("TOUCH_DEVICE", "").strip() or None
TOUCH_DEBUG = os.environ.get("TOUCH_DEBUG", "0") == "1"

TOUCH_SWAP_XY = os.environ.get("TOUCH_SWAP_XY", "0") == "1"
TOUCH_INVERT_X = os.environ.get("TOUCH_INVERT_X", "0") == "1"
TOUCH_INVERT_Y = os.environ.get("TOUCH_INVERT_Y", "0") == "1"


# ============================================================
# SCRenns.py
# ============================================================

from Screnns import (
    Display_MainScreen,
    Display_ScreenMpu,
    Display_ScrennBpm,
    Display_ScrennOFF,
    Display_ScrennON,
)

# O Screnns.py desenha aqui.
# O dummy NÃO conversa com o hardware.
device = dummy(width=WIDTH, height=HEIGHT, mode="RGB")


# ============================================================
# ILI9341
# ============================================================

Pulse = namedtuple(
    "Pulse",
    ["group_bits", "group_mask", "pulse_delay"]
)


class ILI9341:
    def __init__(self):
        print("[DISPLAY] Abrindo gpiochip0...")

        self.h = lgpio.gpiochip_open(GPIO_CHIP)

        self.bus = D_PINS + [PIN_WR]
        self.leader = self.bus[0]

        # D0..D7 + WR
        self.bus_mask = (1 << 9) - 1

        # Grupo de saída.
        # Explicitamente inicializamos todos os níveis.
        levels = [0] * 8 + [1]
        lgpio.group_claim_output(
            self.h,
            self.bus,
            levels
        )

        # Controle.
        lgpio.gpio_claim_output(self.h, PIN_RST, 1)
        lgpio.gpio_claim_output(self.h, PIN_CS, 1)
        lgpio.gpio_claim_output(self.h, PIN_RS, 1)
        lgpio.gpio_claim_output(self.h, PIN_RD, 1)

        # Estados ociosos.
        lgpio.gpio_write(self.h, PIN_CS, 1)
        lgpio.gpio_write(self.h, PIN_RS, 1)
        lgpio.gpio_write(self.h, PIN_RD, 1)

        self._bus(0x00, 1)

        print(
            "[DISPLAY] Pinagem BCM: "
            f"RST={PIN_RST}, CS={PIN_CS}, RS={PIN_RS}, "
            f"WR={PIN_WR}, RD={PIN_RD}, D={D_PINS}"
        )

        self.reset()
        self.init()

    # --------------------------------------------------------
    # Barramento
    # --------------------------------------------------------

    def _bus(self, value, wr):
        bits = (value & 0xFF) | ((1 if wr else 0) << 8)

        lgpio.group_write(
            self.h,
            self.leader,
            bits,
            self.bus_mask
        )

    def _send_bytes_wave(self, data):
        """
        Envia bytes usando lgpio.tx_wave.

        Para cada byte:
          P1: coloca D0..D7 e mantém WR alto
          P2: WR baixo
          P3: WR alto

        1 us por etapa deixa os sinais com margem suficiente para o
        primeiro teste. A atualização de uma tela inteira fica muito
        mais rápida que chamar group_write três vezes por byte.
        """

        if not data:
            return

        # Limite conservador de pulsos por onda.
        # 3 pulsos por byte.
        MAX_BYTES = 3000

        data = bytes(data)

        for start in range(0, len(data), MAX_BYTES):
            chunk = data[start:start + MAX_BYTES]

            pulses = []

            for value in chunk:
                value = value & 0xFF

                # Dados estáveis + WR alto.
                pulses.append(
                    Pulse(
                        value | 0x100,
                        0x1FF,
                        1
                    )
                )

                # Somente WR vai para LOW.
                pulses.append(
                    Pulse(
                        value,
                        0x100,
                        1
                    )
                )

                # WR volta para HIGH.
                pulses.append(
                    Pulse(
                        value | 0x100,
                        0x100,
                        1
                    )
                )

            lgpio.tx_wave(
                self.h,
                self.leader,
                pulses
            )

            # Espera terminar antes de alterar o barramento.
            while lgpio.tx_busy(
                self.h,
                self.leader,
                lgpio.TX_WAVE
            ):
                time.sleep(0.0005)

    def write_cmd(self, cmd):
        lgpio.gpio_write(self.h, PIN_RS, 0)
        lgpio.gpio_write(self.h, PIN_CS, 0)

        self._send_bytes_wave(bytes([cmd]))

        lgpio.gpio_write(self.h, PIN_CS, 1)

    def write_data(self, *values):
        if not values:
            return

        lgpio.gpio_write(self.h, PIN_RS, 1)
        lgpio.gpio_write(self.h, PIN_CS, 0)

        self._send_bytes_wave(bytes(values))

        lgpio.gpio_write(self.h, PIN_CS, 1)

    # --------------------------------------------------------
    # Reset
    # --------------------------------------------------------

    def reset(self):
        print("[DISPLAY] Resetando ILI9341...")

        lgpio.gpio_write(self.h, PIN_CS, 1)
        lgpio.gpio_write(self.h, PIN_RS, 1)
        lgpio.gpio_write(self.h, PIN_RD, 1)

        lgpio.gpio_write(self.h, PIN_RST, 0)
        time.sleep(0.10)

        lgpio.gpio_write(self.h, PIN_RST, 1)
        time.sleep(0.150)

    # --------------------------------------------------------
    # Inicialização
    # --------------------------------------------------------

    def init(self):
        print("[DISPLAY] Inicializando controlador ILI9341...")

        # Software reset
        self.write_cmd(0x01)
        time.sleep(0.150)

        # Power control
        self.write_cmd(0xCF)
        self.write_data(0x00, 0xC1, 0x30)

        self.write_cmd(0xED)
        self.write_data(0x64, 0x03, 0x12, 0x81)

        self.write_cmd(0xE8)
        self.write_data(0x85, 0x00, 0x78)

        self.write_cmd(0xCB)
        self.write_data(0x39, 0x2C, 0x00, 0x34, 0x02)

        self.write_cmd(0xF7)
        self.write_data(0x20)

        self.write_cmd(0xEA)
        self.write_data(0x00, 0x00)

        # Power
        self.write_cmd(0xC0)
        self.write_data(0x23)

        self.write_cmd(0xC1)
        self.write_data(0x10)

        # VCOM
        self.write_cmd(0xC5)
        self.write_data(0x3E, 0x28)

        self.write_cmd(0xC7)
        self.write_data(0x86)

        # Orientação
        self.write_cmd(0x36)
        self.write_data(MADCTL)

        # RGB565
        self.write_cmd(0x3A)
        self.write_data(0x55)

        # Frame rate
        self.write_cmd(0xB1)
        self.write_data(0x00, 0x18)

        # Display function
        self.write_cmd(0xB6)
        self.write_data(0x08, 0x82, 0x27)

        self.write_cmd(0xF2)
        self.write_data(0x00)

        self.write_cmd(0x26)
        self.write_data(0x01)

        # Gamma positiva
        self.write_cmd(0xE0)
        self.write_data(
            0x0F, 0x31, 0x2B, 0x0C,
            0x0E, 0x08, 0x4E, 0xF1,
            0x37, 0x07, 0x10, 0x03,
            0x0E, 0x09, 0x00
        )

        # Gamma negativa
        self.write_cmd(0xE1)
        self.write_data(
            0x00, 0x0E, 0x14, 0x03,
            0x11, 0x07, 0x31, 0xC1,
            0x48, 0x08, 0x0F, 0x0C,
            0x31, 0x36, 0x0F
        )

        # Sleep Out
        self.write_cmd(0x11)
        time.sleep(0.150)

        # Display ON
        self.write_cmd(0x29)
        time.sleep(0.050)

        print("[DISPLAY] ILI9341 inicializado.")
        print("[DISPLAY] NÃO fazendo clear lento do framebuffer.")

    # --------------------------------------------------------
    # Janela
    # --------------------------------------------------------

    def set_window(self, x0, y0, x1, y1):
        self.write_cmd(0x2A)

        self.write_data(
            (x0 >> 8) & 0xFF,
            x0 & 0xFF,
            (x1 >> 8) & 0xFF,
            x1 & 0xFF
        )

        self.write_cmd(0x2B)

        self.write_data(
            (y0 >> 8) & 0xFF,
            y0 & 0xFF,
            (y1 >> 8) & 0xFF,
            y1 & 0xFF
        )

        self.write_cmd(0x2C)

    # --------------------------------------------------------
    # Imagem
    # --------------------------------------------------------

    @staticmethod
    def rgb565(r, g, b):
        color = (
            ((r & 0xF8) << 8)
            | ((g & 0xFC) << 3)
            | (b >> 3)
        )

        return bytes([
            (color >> 8) & 0xFF,
            color & 0xFF
        ])

    def show(self, image):
        image = image.convert("RGB")

        if image.size != (WIDTH, HEIGHT):
            image = image.resize((WIDTH, HEIGHT))

        print("[DISPLAY] Preparando framebuffer RGB565...")

        payload = bytearray()

        pixels = image.load()

        for y in range(HEIGHT):
            for x in range(WIDTH):
                r, g, b = pixels[x, y]

                color = (
                    ((r & 0xF8) << 8)
                    | ((g & 0xFC) << 3)
                    | (b >> 3)
                )

                payload.append((color >> 8) & 0xFF)
                payload.append(color & 0xFF)

        print(
            f"[DISPLAY] Enviando {len(payload)} bytes para ILI9341..."
        )

        self.set_window(
            0,
            0,
            WIDTH - 1,
            HEIGHT - 1
        )

        lgpio.gpio_write(self.h, PIN_RS, 1)
        lgpio.gpio_write(self.h, PIN_CS, 0)

        self._send_bytes_wave(payload)

        lgpio.gpio_write(self.h, PIN_CS, 1)

        print("[DISPLAY] Frame enviado.")

    def close(self):
        if self.h is None:
            return

        try:
            try:
                lgpio.group_free(
                    self.h,
                    self.leader
                )
            except Exception:
                pass

            for gpio in (
                PIN_RST,
                PIN_CS,
                PIN_RS,
                PIN_RD
            ):
                try:
                    lgpio.gpio_free(
                        self.h,
                        gpio
                    )
                except Exception:
                    pass

        finally:
            try:
                lgpio.gpiochip_close(self.h)
            finally:
                self.h = None


# ============================================================
# TOUCH
# ============================================================

class Touch:
    def __init__(self):
        self.dev = None
        self.x_code = None
        self.y_code = None

        self.xmin = 0
        self.xmax = 1
        self.ymin = 0
        self.ymax = 1

        self.raw_x = None
        self.raw_y = None
        self.down = False

        self.find()

    def find(self):
        if InputDevice is None:
            print("[TOUCH] evdev não instalado.")
            return

        paths = (
            [TOUCH_DEVICE]
            if TOUCH_DEVICE
            else list_devices()
        )

        for path in paths:
            try:
                dev = InputDevice(path)
                caps = dev.capabilities(
                    absinfo=True
                )

                axes = dict(
                    caps.get(
                        ecodes.EV_ABS,
                        []
                    )
                )

                if (
                    ecodes.ABS_X in axes
                    and ecodes.ABS_Y in axes
                ):
                    self.x_code = ecodes.ABS_X
                    self.y_code = ecodes.ABS_Y

                elif (
                    ecodes.ABS_MT_POSITION_X in axes
                    and ecodes.ABS_MT_POSITION_Y in axes
                ):
                    self.x_code = ecodes.ABS_MT_POSITION_X
                    self.y_code = ecodes.ABS_MT_POSITION_Y

                else:
                    dev.close()
                    continue

                self.xmin = axes[self.x_code].min
                self.xmax = axes[self.x_code].max
                self.ymin = axes[self.y_code].min
                self.ymax = axes[self.y_code].max

                self.dev = dev

                print(
                    f"[TOUCH] encontrado: "
                    f"{dev.path} - {dev.name}"
                )

                print(
                    f"[TOUCH] X={self.xmin}..{self.xmax} "
                    f"Y={self.ymin}..{self.ymax}"
                )

                return

            except Exception:
                continue

        print(
            "[TOUCH] desativado: nenhum touchscreen "
            "evdev compatível foi encontrado."
        )

    def map_xy(self):
        if (
            self.raw_x is None
            or self.raw_y is None
        ):
            return None

        x = round(
            (self.raw_x - self.xmin)
            * (WIDTH - 1)
            / max(1, self.xmax - self.xmin)
        )

        y = round(
            (self.raw_y - self.ymin)
            * (HEIGHT - 1)
            / max(1, self.ymax - self.ymin)
        )

        x = max(0, min(WIDTH - 1, x))
        y = max(0, min(HEIGHT - 1, y))

        if TOUCH_SWAP_XY:
            x, y = y, x

        if TOUCH_INVERT_X:
            x = WIDTH - 1 - x

        if TOUCH_INVERT_Y:
            y = HEIGHT - 1 - y

        return x, y

    def read(self):
        if self.dev is None:
            return []

        clicks = []

        try:
            for event in self.dev.read():

                if event.type == ecodes.EV_ABS:

                    if event.code == self.x_code:
                        self.raw_x = event.value

                    elif event.code == self.y_code:
                        self.raw_y = event.value

                elif (
                    event.type == ecodes.EV_KEY
                    and event.code == ecodes.BTN_TOUCH
                ):

                    if event.value:
                        self.down = True

                    elif self.down:
                        point = self.map_xy()

                        if point:
                            clicks.append(point)

                        self.down = False

                if TOUCH_DEBUG:
                    print(
                        "[TOUCH]",
                        event.type,
                        event.code,
                        event.value
                    )

        except OSError:
            pass

        return clicks

    def close(self):
        if self.dev:
            self.dev.close()
            self.dev = None


# ============================================================
# DADOS
# ============================================================

def obter_dados_do_c():
    try:
        resultado = subprocess.run(
            [C_PROGRAM],
            capture_output=True,
            text=True,
            check=True,
            timeout=0.8
        )

        if not resultado.stdout.strip():
            return {}

        return json.loads(
            resultado.stdout
        )

    except Exception:
        return {}


def preparar_dados(data):
    if not isinstance(data, dict):
        data = {}

    bmp = data.get("bmp", {})
    mpu = data.get("mpu", {})

    if not isinstance(bmp, dict):
        bmp = {}

    if not isinstance(mpu, dict):
        mpu = {}

    sensor_mpu = {
        "status": str(
            mpu.get("status", "online")
        ).upper(),

        "direcao": (
            f"{mpu.get('direcao', 0)} deg"
        ),

        "velocidade": (
            f"{mpu.get('velocidade', 0)} m/s"
        ),

        "estado": str(
            mpu.get("estadoMPU", "normal")
        ).upper()
    }

    sensor_bmp = {
        "status": str(
            bmp.get("status", "offline")
        ).upper(),

        "pressao": (
            f"{bmp.get('pressao', 0)} hPa"
        ),

        "temperatura": (
            f"{bmp.get('temperatura', 0)}° C"
        ),

        "altitude": (
            f"{bmp.get('altitude', 0)} m"
        ),

        "estado": str(
            bmp.get("estadoBPM", "normal")
        ).upper()
    }

    return sensor_mpu, sensor_bmp


# ============================================================
# NAVEGAÇÃO
# ============================================================

tela_atual = "MAIN"


def Display_TrateClick(x, y):
    global tela_atual

    if tela_atual == "MAIN":

        if 10 <= x <= 155 and 40 <= y <= 160:
            tela_atual = "BMP"

        elif 165 <= x <= 310 and 40 <= y <= 160:
            tela_atual = "MPU"

        elif 10 <= x <= 310 and 180 <= y <= 230:
            tela_atual = "OFF"

    elif tela_atual in (
        "BMP",
        "MPU",
        "OFF"
    ):

        if 150 <= x <= 320 and 0 <= y <= 40:
            tela_atual = "MAIN"

        elif tela_atual == "OFF":

            if 30 <= x <= 145 and 165 <= y <= 210:
                print(
                    "Desligando o sistema..."
                )
                tela_atual = "ON"

            elif 175 <= x <= 290 and 165 <= y <= 210:
                tela_atual = "MAIN"

    elif tela_atual == "ON":

        if 60 <= x <= 260 and 95 <= y <= 145:
            tela_atual = "MAIN"


# ============================================================
# RENDER
# ============================================================

def render(sensor_mpu, sensor_bmp):

    if tela_atual == "MAIN":
        Display_MainScreen(
            device,
            sensor_mpu,
            sensor_bmp
        )

    elif tela_atual == "BMP":
        Display_ScrennBpm(
            device,
            sensor_bmp
        )

    elif tela_atual == "MPU":
        Display_ScreenMpu(
            device,
            sensor_mpu
        )

    elif tela_atual == "OFF":
        Display_ScrennOFF(
            device
        )

    elif tela_atual == "ON":
        Display_ScrennON(
            device
        )


# ============================================================
# MAIN
# ============================================================

display = None
touch = None


def cleanup(*args):
    global display, touch

    if touch:
        try:
            touch.close()
        except Exception:
            pass

        touch = None

    if display:
        try:
            display.close()
        except Exception:
            pass

        display = None


def stop_handler(signum, frame):
    cleanup()
    raise SystemExit(0)


signal.signal(
    signal.SIGINT,
    stop_handler
)

signal.signal(
    signal.SIGTERM,
    stop_handler
)


def main():

    global display
    global touch
    global tela_atual

    print("====================================")
    print(" MAR2406 / ILI9341")
    print(" Raspberry Pi 5")
    print("====================================")

    display = ILI9341()

    touch = Touch()

    tela_atual = "MAIN"

    sensor_mpu, sensor_bmp = (
        preparar_dados(
            obter_dados_do_c()
        )
    )

    # --------------------------------------------------------
    # PRIMEIRO FRAME
    # --------------------------------------------------------

    print(
        "[DISPLAY] Renderizando MAIN "
        "através do Screnns.py..."
    )

    render(
        sensor_mpu,
        sensor_bmp
    )

    print(
        "[DISPLAY] Enviando MAIN "
        "para o ILI9341..."
    )

    display.show(
        device.image
    )

    print(
        "[DISPLAY] MAIN exibida."
    )

    ultima_tela = tela_atual

    ultima_assinatura = (
        repr(sensor_mpu),
        repr(sensor_bmp)
    )

    ultimo_update = time.monotonic()

    # --------------------------------------------------------
    # LOOP
    # --------------------------------------------------------

    while True:

        # Touch
        if touch and touch.dev:

            try:

                ready, _, _ = select.select(
                    [touch.dev.fd],
                    [],
                    [],
                    0
                )

                if ready:

                    for x, y in touch.read():

                        print(
                            f"[TOUCH] x={x} y={y}"
                        )

                        Display_TrateClick(
                            x,
                            y
                        )

            except (
                OSError,
                ValueError
            ):
                pass

        # Dados
        now = time.monotonic()

        dados_mudaram = False

        if (
            now - ultimo_update
            >= DATA_INTERVAL
        ):

            ultimo_update = now

            novo_mpu, novo_bmp = (
                preparar_dados(
                    obter_dados_do_c()
                )
            )

            assinatura = (
                repr(novo_mpu),
                repr(novo_bmp)
            )

            if assinatura != ultima_assinatura:

                sensor_mpu = novo_mpu
                sensor_bmp = novo_bmp

                ultima_assinatura = assinatura

                dados_mudaram = True

        # Troca de tela ou dados
        if (
            tela_atual != ultima_tela
            or dados_mudaram
        ):

            render(
                sensor_mpu,
                sensor_bmp
            )

            display.show(
                device.image
            )

            ultima_tela = tela_atual

        time.sleep(0.005)


if __name__ == "__main__":

    try:
        main()

    except KeyboardInterrupt:
        cleanup()

    except Exception as exc:
        cleanup()

        print(
            f"[ERRO] {exc}",
            file=sys.stderr
        )

        raise
