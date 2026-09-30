"""
Loop principal do sistema de monitoramento.

Navegação por 2 botões físicos (sem touch):
- BTN_NAV   : percorre as opções disponíveis na tela atual
- BTN_ENTER : confirma a opção selecionada

Fluxo das telas:
  ON  -> (ENTER) -> MAIN
  MAIN [BMP280 / MPU6050 / DESLIGAR] -> (NAV move seleção, ENTER escolhe)
      BMP280   -> BPM (tela de detalhes)  -> ENTER volta pra MAIN
      MPU6050  -> MPU (tela de detalhes)  -> ENTER volta pra MAIN
      DESLIGAR -> OFF_CONFIRM [SIM / NAO]
          SIM -> desliga (aqui você decide o que fazer: os.system("sudo shutdown now"), etc)
          NAO -> volta pra MAIN
"""

import time
import lgpio

import Screnns
from ili9341_parallel import ILI9341Parallel

# ---------------- Pinagem dos botões (BCM) ----------------
BTN_NAV = 23    # navega entre as opções
BTN_ENTER = 24  # confirma

GPIOCHIP = 4    # RP1 na Raspberry Pi 5 (use 0 numa Pi 4 ou anterior)
DEBOUNCE_US = 30_000  # 30ms de debounce

# ---------------- Estados ----------------
ON, MAIN, MPU, BPM, OFF_CONFIRM = "ON", "MAIN", "MPU", "BPM", "OFF_CONFIRM"


def ler_dados_sensores():
    """
    TODO: troque isso pela integração real com a parte de captura de sensores
    do seu grupo (arquivo compartilhado, fila, socket, MQTT, serial, etc).
    Por enquanto retorna valores fixos só para o display funcionar sozinho.
    """
    sensor_mpu = {
        "status": "online",
        "direcao": "N 45°",
        "velocidade": "12.3 km/h",
        "estado": "normal",
    }
    sensor_bpm = {
        "status": "online",
        "pressao": "1013 hPa",
        "temperatura": "24.5 C",
        "altitude": "812 m",
        "estado": "normal",
    }
    return sensor_mpu, sensor_bpm


class Botoes:
    def __init__(self):
        self.h = lgpio.gpiochip_open(GPIOCHIP)
        for pin in (BTN_NAV, BTN_ENTER):
            lgpio.gpio_claim_input(self.h, pin, lgpio.SET_PULL_UP)
            lgpio.gpio_set_debounce_micros(self.h, pin, DEBOUNCE_US)
        self._ultimo = {BTN_NAV: 1, BTN_ENTER: 1}

    def pressionado(self, pin):
        """Retorna True uma única vez, na borda de descida (botão pressionado)."""
        atual = lgpio.gpio_read(self.h, pin)
        borda = (self._ultimo[pin] == 1 and atual == 0)
        self._ultimo[pin] = atual
        return borda

    def close(self):
        lgpio.gpiochip_close(self.h)


def main():
    device = ILI9341Parallel()
    botoes = Botoes()

    estado = ON
    selecionado_main = 0     # 0=BMP280 | 1=MPU6050 | 2=DESLIGAR
    selecionado_off = 1      # 0=SIM | 1=NAO

    Screnns.Display_ScrennON(device)

    try:
        while True:
            nav = botoes.pressionado(BTN_NAV)
            enter = botoes.pressionado(BTN_ENTER)

            if estado == ON:
                if enter:
                    estado = MAIN
                    sensor_mpu, sensor_bpm = ler_dados_sensores()
                    Screnns.Display_MainScreen(device, sensor_mpu, sensor_bpm, selecionado_main)

            elif estado == MAIN:
                sensor_mpu, sensor_bpm = ler_dados_sensores()
                if nav:
                    selecionado_main = (selecionado_main + 1) % 3
                    Screnns.Display_MainScreen(device, sensor_mpu, sensor_bpm, selecionado_main)
                elif enter:
                    if selecionado_main == 0:
                        estado = BPM
                        Screnns.Display_ScrennBpm(device, sensor_bpm)
                    elif selecionado_main == 1:
                        estado = MPU
                        Screnns.Display_ScreenMpu(device, sensor_mpu)
                    else:
                        estado = OFF_CONFIRM
                        selecionado_off = 1
                        Screnns.Display_ScrennOFF(device, selecionado_off)

            elif estado == BPM:
                if enter:
                    estado = MAIN
                    sensor_mpu, sensor_bpm = ler_dados_sensores()
                    Screnns.Display_MainScreen(device, sensor_mpu, sensor_bpm, selecionado_main)

            elif estado == MPU:
                if enter:
                    estado = MAIN
                    sensor_mpu, sensor_bpm = ler_dados_sensores()
                    Screnns.Display_MainScreen(device, sensor_mpu, sensor_bpm, selecionado_main)

            elif estado == OFF_CONFIRM:
                if nav:
                    selecionado_off = 1 - selecionado_off
                    Screnns.Display_ScrennOFF(device, selecionado_off)
                elif enter:
                    if selecionado_off == 0:  # SIM
                        # TODO: aqui entra a ação real de desligar
                        # ex: os.system("sudo shutdown -h now")
                        Screnns.Display_ScrennON(device)
                        estado = ON
                    else:  # NAO
                        estado = MAIN
                        sensor_mpu, sensor_bpm = ler_dados_sensores()
                        Screnns.Display_MainScreen(device, sensor_mpu, sensor_bpm, selecionado_main)

            time.sleep(0.02)  # ~50Hz de polling dos botões

    except KeyboardInterrupt:
        pass
    finally:
        botoes.close()
        device.close()


if __name__ == "__main__":
    main()
