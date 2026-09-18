import os
from PIL import ImageFont
from luma.core.render import canvas

# Carrega a fonte Ubuntu para os textos
fonte_titulo = ImageFont.truetype("Ubuntu-Regular.ttf", 18)
fonte_texto = ImageFont.truetype("Ubuntu-Regular.ttf", 15)
fonte_valores = ImageFont.truetype("Ubuntu-Regular.ttf", 15)
fonte_sensor = ImageFont.truetype("Ubuntu-Regular.ttf", 22)


# Carrega fontes do sistema com suporte aos símbolos Unicode (⏻, ⏲, 🌡, ❮)
def carregar_fonte_simbolos(tamanho):
    fontes_candidatas = [
        "DejaVuSans.ttf",
        "Symbola.ttf",
        "FreeMono.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/freefont/FreeMono.ttf",
        "arial.ttf",
        "seguiemj.ttf"
    ]
    for nome in fontes_candidatas:
        try:
            return ImageFont.truetype(nome, tamanho)
        except (OSError, IOError):
            continue
    return ImageFont.load_default()

fonte_simbolo = carregar_fonte_simbolos(15)
# ----------------------------------------------------------------------------

# Cores
COR_FUNDO = "#121212"
COR_CAIXA = "#2A2A35"
COR_TEXTO = "#FFFFFF"
COR_ROTULO = "#E4E4E4"


def Display_MainScreen(device, SensorMPU, SensorBPM):
    with canvas(device) as draw: 
        # BACKGROUND DE FUNDO
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline=None)

        # CABEÇALHO
        draw.text((160, 16), "SISTEMA  DE  MONITORAMENTO", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")

        
        corBPM = "#00FFAA" if SensorBPM["status"].lower() != "offline" else "#FF3366"
        corMPU = "#00FFAA" if SensorMPU["status"].lower() != "offline" else "#FF3366"

        # BOX SENSOR 1 (BMP280)
        draw.rounded_rectangle((10, 40, 155, 160), fill=COR_CAIXA, outline=None, radius=10)
        draw.ellipse((18, 48, 30, 60), fill=corBPM)     
        draw.text((82, 100), "BMP280", fill=COR_TEXTO, font=fonte_sensor, anchor="mm")  

        # BOX SENSOR 2 (MPU6050)
        draw.rounded_rectangle((165, 40, 310, 160), fill=COR_CAIXA, outline=None, radius=10)
        draw.ellipse((173, 48, 185, 60), fill=corMPU)  
        draw.text((237, 100), "MPU6050", fill=COR_TEXTO, font=fonte_sensor, anchor="mm")  

        # BOTÃO DE DESLIGAR
        draw.rounded_rectangle((10, 180, 310, 230), fill=COR_CAIXA, outline=None, radius=10)
        draw.text((160, 205), "DESLIGAR SISTEMA", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")


def Display_ScreenMpu(device, SensorMPU):
    with canvas(device) as draw:
        # BACKGROUND DE FUNDO 
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline=None)

        # CABEÇALHO (com símbolo ❮)
        draw.text((15, 6), "MPU6050", fill=COR_TEXTO, font=fonte_titulo)
        draw.text((220, 4), "❮", fill=COR_TEXTO, font=fonte_simbolo)
        draw.text((240, 6), "VOLTAR", fill=COR_TEXTO, font=fonte_titulo)

        # MOSTRA DOS VALORES CAPTADOS PELO SENSOR
        draw.text((30, 75), "DIRECAO:", fill=COR_ROTULO, font=fonte_texto)
        draw.rounded_rectangle((160, 70, 290, 95), fill=COR_CAIXA, outline=None, radius=4)
        draw.text((168, 75), SensorMPU["direcao"], fill=COR_TEXTO, font=fonte_valores)

        draw.text((30, 135), "VELOCIDADE:", fill=COR_ROTULO, font=fonte_texto)
        draw.rounded_rectangle((160, 130, 290, 155), fill=COR_CAIXA, outline=None, radius=4)
        draw.text((168, 135), SensorMPU["velocidade"], fill=COR_TEXTO, font=fonte_valores)

        # MOSTRA DO ESTADO VISUAL (RODAPE)
        EstadoMPU = "#00FFAA" if SensorMPU["estado"].lower() != "anormal" else "#FF3366"
        draw.line((0, 195, 320, 195), fill=COR_CAIXA, width=2)
        draw.text((30, 207), "ESTADO:", fill=COR_TEXTO, font=fonte_texto)
        draw.text((100, 207), f"{SensorMPU['estado']}", fill=EstadoMPU, font=fonte_texto)


def Display_ScrennBpm(device, SensorBPM):
    with canvas(device) as draw:
        # BACKGROUND DE FUNDO
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline=None)

        # CABEÇALHO (com símbolo ❮)
        draw.text((15, 6), "BMP280", fill=COR_TEXTO, font=fonte_titulo)
        draw.text((220, 4), "❮", fill=COR_TEXTO, font=fonte_simbolo)
        draw.text((240, 6), "VOLTAR", fill=COR_TEXTO, font=fonte_titulo)

        # MOSTRA DOS VALORES CAPTADOS PELO SENSOR
        draw.text((30, 55), "PRESSAO:", fill=COR_ROTULO, font=fonte_texto)
        draw.rounded_rectangle((160, 50, 290, 75), fill=COR_CAIXA, outline=None, radius=4)
        draw.text((168, 55), SensorBPM["pressao"], fill=COR_TEXTO, font=fonte_valores)

        draw.text((30, 105), "TEMPERATURA:", fill=COR_ROTULO, font=fonte_texto)
        draw.rounded_rectangle((160, 100, 290, 125), fill=COR_CAIXA, outline=None, radius=4)
        draw.text((168, 105), SensorBPM["temperatura"], fill=COR_TEXTO, font=fonte_valores)

        draw.text((30, 155), "ALTITUDE:", fill=COR_ROTULO, font=fonte_texto)
        draw.rounded_rectangle((160, 150, 290, 175), fill=COR_CAIXA, outline=None, radius=4)
        draw.text((168, 155), SensorBPM["altitude"], fill=COR_TEXTO, font=fonte_valores)

        # MOSTRA DO ESTADO VISUAL (RODAPE)
        EstadoBPM = "#00FFAA" if SensorBPM["estado"].lower() != "anormal" else "#FF3366"
        draw.line((0, 195, 320, 195), fill=COR_CAIXA, width=2)
        draw.text((30, 207), "ESTADO:", fill=COR_TEXTO, font=fonte_texto)
        draw.text((100, 207), f"{SensorBPM['estado']}", fill=EstadoBPM, font=fonte_texto)


def Display_ScrennOFF(device):
    with canvas(device) as draw:
        # BACKGROUND DE FUNDO
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline=None)

        draw.text((220, 4), "❮", fill=COR_TEXTO, font=fonte_simbolo)
        draw.text((240, 6), "VOLTAR", fill=COR_TEXTO, font=fonte_titulo)

        draw.text((160, 100), "TEM CERTEZA QUE DESEJA", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")
        draw.text((160, 122), "DESLIGAR O SISTEMA?", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")

        # BOTÕES DE SIM E NÃO
        draw.rounded_rectangle((30, 165, 145, 210), fill=COR_CAIXA, outline=None, radius=8)
        draw.text((87, 187), "SIM", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")
        draw.rounded_rectangle((175, 165, 290, 210), fill=COR_CAIXA, outline=None, radius=8)
        draw.text((232, 187), "NÃO", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")


def Display_ScrennON(device):
    with canvas(device) as draw:
        # BACKGROUND DE FUNDO
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline=None)
    
        draw.rounded_rectangle((60, 95, 260, 145), fill=COR_CAIXA, outline=None, radius=10)
        draw.text((160, 120), "LIGAR SISTEMA", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")
        