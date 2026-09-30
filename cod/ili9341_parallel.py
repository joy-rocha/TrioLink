"""
Driver ILI9341 em modo paralelo 8 bits (8080-I) para Raspberry Pi 5, usando lgpio.

Expõe uma interface compatível com o que o luma.core.render.canvas espera de um
"device": atributos .mode / .size e o método .display(image). Assim, o
Screnns.py original (que usa `with canvas(device) as draw:`) funciona sem
qualquer alteração na lógica de desenho.

IMPORTANTE:
- Na Raspberry Pi 5 a GPIO fica no chip RP1, acessível em /dev/gpiochip4.
- LCD_RD do shield deve ser ligado FISICAMENTE no 3V3 (não usamos leitura,
  então não gastamos um GPIO com isso).
- Esse é um barramento "bit-banged" (não é hardware nativo), então uma
  atualização de tela cheia (320x240) leva uma fração de segundo — é
  tranquilo para um painel de monitoramento que não precisa de vídeo fluido.
"""

import time
import numpy as np
import lgpio

# ---------------- Pinagem (BCM) ----------------
PIN_RST = 4
PIN_CS = 17
PIN_RS = 27   # D/C: 0 = comando, 1 = dado
PIN_WR = 22
PINS_DATA = [5, 6, 12, 13, 16, 19, 20, 21]  # D0..D7, nessa ordem

GPIOCHIP = 4  # RP1 na Raspberry Pi 5 (use 0 se estiver rodando numa Pi 4 ou anterior)


class ILI9341Parallel:
    def __init__(self, width=320, height=240, rotation=0xE8):
        self.width = width
        self.height = height
        self.size = (width, height)
        self.mode = "RGB"

        self.h = lgpio.gpiochip_open(GPIOCHIP)

        lgpio.gpio_claim_output(self.h, PIN_RST, 1)
        lgpio.gpio_claim_output(self.h, PIN_CS, 0)   # CS sempre em nível baixo (único dispositivo no barramento)
        lgpio.gpio_claim_output(self.h, PIN_RS, 1)
        lgpio.gpio_claim_output(self.h, PIN_WR, 1)
        # ATENÇÃO: nessa versão da lgpio a ordem é (handle, gpio, levels, lFlags) —
        # os níveis vêm antes das flags, o inverso do que eu tinha colocado antes.
        lgpio.group_claim_output(self.h, PINS_DATA, [0] * 8)

        self._reset()
        self._init_sequence(rotation)

    # ---------------- Baixo nível ----------------
    def _pulse_wr(self):
        lgpio.gpio_write(self.h, PIN_WR, 0)
        lgpio.gpio_write(self.h, PIN_WR, 1)

    def _write_byte(self, value):
        # group_write identifica o grupo pelo seu GPIO líder (o primeiro
        # passado em group_claim_output), não pela lista inteira.
        lgpio.group_write(self.h, PINS_DATA[0], value)
        self._pulse_wr()

    def _write_cmd(self, cmd, *data):
        lgpio.gpio_write(self.h, PIN_RS, 0)
        self._write_byte(cmd)
        if data:
            lgpio.gpio_write(self.h, PIN_RS, 1)
            for d in data:
                self._write_byte(d)

    def _write_data_bytes(self, byte_iterable):
        lgpio.gpio_write(self.h, PIN_RS, 1)
        for b in byte_iterable:
            self._write_byte(b)

    def _reset(self):
        lgpio.gpio_write(self.h, PIN_RST, 1)
        time.sleep(0.01)
        lgpio.gpio_write(self.h, PIN_RST, 0)
        time.sleep(0.02)
        lgpio.gpio_write(self.h, PIN_RST, 1)
        time.sleep(0.15)

    def _init_sequence(self, rotation):
        c = self._write_cmd
        c(0xEF, 0x03, 0x80, 0x02)
        c(0xCF, 0x00, 0xC1, 0x30)
        c(0xED, 0x64, 0x03, 0x12, 0x81)
        c(0xE8, 0x85, 0x00, 0x78)
        c(0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02)
        c(0xF7, 0x20)
        c(0xEA, 0x00, 0x00)
        c(0xC0, 0x23)              # Power control 1
        c(0xC1, 0x10)              # Power control 2
        c(0xC5, 0x3E, 0x28)        # VCOM control 1
        c(0xC7, 0x86)              # VCOM control 2
        c(0x36, rotation)          # Memory Access Control (orientação)
        c(0x3A, 0x55)              # Pixel format: 16 bits RGB565
        c(0xB1, 0x00, 0x18)        # Frame rate
        c(0xB6, 0x08, 0x82, 0x27)  # Display function control
        c(0xF2, 0x00)
        c(0x26, 0x01)
        c(0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00)
        c(0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F)
        c(0x11)                    # Sleep out
        time.sleep(0.12)
        c(0x29)                    # Display on
        time.sleep(0.02)

    def _set_window(self, x0, y0, x1, y1):
        self._write_cmd(0x2A, x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF)  # CASET
        self._write_cmd(0x2B, y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF)  # PASET
        self._write_cmd(0x2C)                                          # RAMWR

    # ---------------- Interface usada pelo luma.core.render.canvas ----------------
    def display(self, image):
        """Recebe uma imagem PIL (RGB, tamanho self.size) e desenha na tela inteira."""
        if image.size != self.size:
            image = image.resize(self.size)
        image = image.convert("RGB")

        self._set_window(0, 0, self.width - 1, self.height - 1)

        arr = np.asarray(image, dtype=np.uint16)
        r = (arr[:, :, 0] & 0xF8) << 8
        g = (arr[:, :, 1] & 0xFC) << 3
        b = (arr[:, :, 2] & 0xF8) >> 3
        rgb565 = r | g | b

        hi = (rgb565 >> 8).astype(np.uint8)
        lo = (rgb565 & 0xFF).astype(np.uint8)
        interleaved = np.dstack((hi, lo)).flatten()

        self._write_data_bytes(interleaved.tolist())

    def close(self):
        lgpio.gpiochip_close(self.h)
