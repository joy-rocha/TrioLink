import lgpio
import time

# Pinos BCM configurados
PIN_RST = 17
PIN_CS  = 27
PIN_RS  = 22
PIN_WR  = 23
PIN_RD  = 24
D_PINS  = [5, 6, 12, 13, 16, 19, 20, 21]

chip = None
for c in [4, 0, 1, 2, 3]:
    try:
        chip = lgpio.gpiochip_open(c)
        print(f"[TESTE] Chip GPIO {c} aberto com sucesso.")
        break
    except Exception:
        pass

if chip is None:
    print("[ERRO] Nenhum chip GPIO encontrado.")
    exit()

# Reivindicar pinos
for p in [PIN_RD, PIN_WR, PIN_RS, PIN_RST, PIN_CS] + D_PINS:
    try:
        lgpio.gpio_claim_output(chip, p)
    except Exception:
        pass

# Manter RD sempre em nível ALTO (1)
lgpio.gpio_write(chip, PIN_RD, 1)

def pulso_wr():
    lgpio.gpio_write(chip, PIN_WR, 0)
    time.sleep(0.00001) # 10 microsegundos de tempo de trava (strobe)
    lgpio.gpio_write(chip, PIN_WR, 1)
    time.sleep(0.00001)

def write_byte(val):
    for i in range(8):
        lgpio.gpio_write(chip, D_PINS[i], (val >> i) & 1)
    pulso_wr()

def write_cmd(cmd):
    lgpio.gpio_write(chip, PIN_RS, 0)
    lgpio.gpio_write(chip, PIN_CS, 0)
    write_byte(cmd)
    lgpio.gpio_write(chip, PIN_CS, 1)

def write_data(data):
    lgpio.gpio_write(chip, PIN_RS, 1)
    lgpio.gpio_write(chip, PIN_CS, 0)
    write_byte(data)
    lgpio.gpio_write(chip, PIN_CS, 1)

print("[TESTE] Efetuando Reset por Hardware...")
lgpio.gpio_write(chip, PIN_RST, 1)
time.sleep(0.05)
lgpio.gpio_write(chip, PIN_RST, 0)
time.sleep(0.1)
lgpio.gpio_write(chip, PIN_RST, 1)
time.sleep(0.2)

# Reset por Software e comandos básicos
write_cmd(0x01) # SWRESET
time.sleep(0.2)

write_cmd(0x3A); write_data(0x55) # RGB565
write_cmd(0x11) # Sleep Out
time.sleep(0.15)
write_cmd(0x29) # Display ON
time.sleep(0.05)

print("[TESTE] Tentando pintar a tela de VERMELHO...")
write_cmd(0x2A); write_data(0x00); write_data(0x00); write_data(0x01); write_data(0x3F) # Colunas
write_cmd(0x2B); write_data(0x00); write_data(0x00); write_data(0x00); write_data(0xEF) # Linhas
write_cmd(0x2C) # RAM Write

lgpio.gpio_write(chip, PIN_RS, 1)
lgpio.gpio_write(chip, PIN_CS, 0)

# Envia 320x240 pixels na cor vermelha (0xF800 em RGB565)
for _ in range(320 * 240):
    write_byte(0xF8)
    write_byte(0x00)

lgpio.gpio_write(chip, PIN_CS, 1)
print("[TESTE] Envio concluído. A tela mudou de cor?")
lgpio.gpiochip_close(chip)
