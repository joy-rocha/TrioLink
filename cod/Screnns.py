import os
from PIL import ImageFont
from luma.core.render import canvas

# Carrega a fonte VT323 para os textos
fonte_titulo = ImageFont.truetype("VT323-Regular.ttf", 22)
fonte_texto = ImageFont.truetype("VT323-Regular.ttf", 20)
fonte_valores = ImageFont.truetype("VT323-Regular.ttf", 26)


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

fonte_simbolo = carregar_fonte_simbolos(20)
fonte_simbolo_grande = carregar_fonte_simbolos(55)
# ----------------------------------------------------------------------------

# Cores
COR_FUNDO = "#0033CC"  
COR_CAIXA = "#001F7A"    
COR_TEXTO = "white" 
COR_STATUS = "#1df700"  


def Display_MainScreen(device, SensorMPU, SensorBPM):
    with canvas(device) as draw: 
        # BACKGROUND DE FUNDO
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline="white")

        # CABEÇALHO
        draw.text((160, 16), "SISTEMA DE MONITORAMENTO", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")
        draw.line((0, 30, 320, 30), fill=COR_TEXTO, width=2)

        # CONFIGURAÇÃO DA IDENTIFICAÇÃO DE CONEXÃO (COR)
        corBPM = "#1df700" if SensorBPM["status"].lower() != "offline" else "#e31c1c"
        corMPU = "#1df700" if SensorMPU["status"].lower() != "offline" else "#e31c1c"

        # BOX SENSOR 1 (BMP280)
        draw.rectangle((10, 40, 155, 160), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.ellipse((18, 48, 30, 60), fill=corBPM)     
        draw.text((82, 100), "BMP280", fill=COR_TEXTO, font=fonte_valores, anchor="mm")  

        # BOX SENSOR 2 (MPU6050)
        draw.rectangle((165, 40, 310, 160), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.ellipse((173, 48, 185, 60), fill=corMPU)  
        draw.text((237, 100), "MPU6050", fill=COR_TEXTO, font=fonte_valores, anchor="mm")  

        # BOTÃO DE DESLIGAR (com símbolo ⏻)
        draw.rectangle((10, 180, 310, 230), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((50, 192), "⏻", fill=COR_TEXTO, font=fonte_simbolo)
        draw.text((80, 195), "DESLIGAR SISTEMA  >", fill=COR_TEXTO, font=fonte_titulo)


def Display_ScreenMpu(device, SensorMPU):
    with canvas(device) as draw:
        # BACKGROUND DE FUNDO 
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline="white")

        # CABEÇALHO (com símbolo ❮)
        draw.text((15, 6), "MPU6050", fill=COR_TEXTO, font=fonte_titulo)
        draw.text((220, 4), "❮", fill=COR_TEXTO, font=fonte_simbolo)
        draw.text((240, 6), "VOLTAR", fill=COR_TEXTO, font=fonte_titulo)
        draw.line((0, 30, 320, 30), fill=COR_TEXTO, width=2)

        # BOX COM ÍCONE ⏲
        draw.rectangle((10, 40, 110, 180), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((60, 110), "⏲", fill="white", font=fonte_simbolo_grande, anchor="mm")

        # MOSTRA DOS VALORES CAPTADOS PELO SENSOR 
        draw.text((120, 65), "DIRECAO:", fill=COR_TEXTO, font=fonte_texto)
        draw.rectangle((210, 63, 310, 87), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((215, 65), SensorMPU["direcao"], fill=COR_TEXTO, font=fonte_valores)

        draw.text((120, 125), "VELOCIDADE:", fill=COR_TEXTO, font=fonte_texto)
        draw.rectangle((210, 123, 310, 147), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((215, 125), SensorMPU["velocidade"], fill=COR_TEXTO, font=fonte_valores)

        # MOSTRA DO ESTADO VISUAL (RODAPE)
        EstadoMPU = "#1df700" if SensorMPU["estado"].lower() != "anormal" else "#e31c1c"
        draw.line((0, 195, 320, 195), fill=COR_TEXTO, width=2)
        draw.text((15, 205), "ESTADO:", fill=COR_TEXTO, font=fonte_titulo)
        draw.text((95, 205), f"{SensorMPU['estado']}", fill=EstadoMPU, font=fonte_titulo)


def Display_ScrennBpm(device, SensorBPM):
    with canvas(device) as draw:
        # BACKGROUND DE FUNDO
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline="white")

        # CABEÇALHO (com símbolo ❮)
        draw.text((15, 6), "BMP280", fill=COR_TEXTO, font=fonte_titulo)
        draw.text((220, 4), "❮", fill=COR_TEXTO, font=fonte_simbolo)
        draw.text((240, 6), "VOLTAR", fill=COR_TEXTO, font=fonte_titulo)
        draw.line((0, 30, 320, 30), fill=COR_TEXTO, width=2)

        # BOX COM ÍCONE 🌡
        draw.rectangle((10, 40, 110, 180), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((60, 110), "🌡", fill="white", font=fonte_simbolo_grande, anchor="mm")

        # MOSTRA DOS VALORES CAPTADOS PELO SENSOR 
        draw.text((120, 50), "PRESSAO:", fill=COR_TEXTO, font=fonte_texto)
        draw.rectangle((210, 48, 310, 72), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((215, 50), SensorBPM["pressao"], fill=COR_TEXTO, font=fonte_valores)

        draw.text((120, 95), "TEMPERATURA:", fill=COR_TEXTO, font=fonte_texto)
        draw.rectangle((210, 93, 310, 117), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((215, 95), SensorBPM["temperatura"], fill=COR_TEXTO, font=fonte_valores)

        draw.text((120, 140), "ALTITUDE:", fill=COR_TEXTO, font=fonte_texto)
        draw.rectangle((210, 138, 310, 162), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((215, 140), SensorBPM["altitude"], fill=COR_TEXTO, font=fonte_valores)

        # MOSTRA DO ESTADO VISUAL (RODAPE)
        EstadoBPM = "#1df700" if SensorBPM["estado"].lower() != "anormal" else "#e31c1c"
        draw.line((0, 195, 320, 195), fill=COR_TEXTO, width=2)
        draw.text((15, 205), "ESTADO:", fill=COR_TEXTO, font=fonte_titulo)
        draw.text((95, 205), f"{SensorBPM['estado']}", fill=EstadoBPM, font=fonte_titulo)


def Display_ScrennOFF(device):
    with canvas(device) as draw:
        # BACKGROUND DE FUNDO
        draw.rectangle((0, 0, 320, 240), fill=COR_FUNDO, outline="white")

        draw.text((15, 6), "DESLIGAR SISTEMA", fill=COR_TEXTO, font=fonte_titulo)
        draw.text((220, 4), "❮", fill=COR_TEXTO, font=fonte_simbolo)
        draw.text((240, 6), "VOLTAR", fill=COR_TEXTO, font=fonte_titulo)
        draw.line((0, 30, 320, 30), fill=COR_TEXTO, width=2)

        draw.text((160, 100), "TEM CERTEZA QUE DESEJA", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")
        draw.text((160, 122), "DESLIGAR O SISTEMA?", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")

        # BOTÕES DE SIM E NÃO
        draw.rectangle((30, 165, 145, 210), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((87, 187), "SIM", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")
        draw.rectangle((175, 165, 290, 210), fill=COR_CAIXA, outline=COR_TEXTO)
        draw.text((232, 187), "NÃO", fill=COR_TEXTO, font=fonte_titulo, anchor="mm")