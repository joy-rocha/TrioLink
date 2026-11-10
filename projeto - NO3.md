O projeto consiste em 3 unidades de  Raspberry pi 5 uma conectada a um sensor MPU, uma coctadaa um sensor BMP e outra conectada a um display, a tela deve mostrar os dados coletados pelos sensores e elas vão se comunicar via MQTT. Os dados vão ser recebidos em formato Json como definidos no arquivo de Schema.md.

O que quero pra este projeto é um sistema simples e funcional no padrão ASSERT, sua linguagem deve ser 100% em C. Também deve ter o uso das libs descritas mais abaixo.

os botões devem ser da seguinte forma, o de SELEÇÃO vai meio que selecionar um dos 3 botões da tela e vai trocando pro outro quando clico, já o de ENTER vai confirmar o botão que ta selecionado e manda pra outra tela correspondente.

as telas do projeto vão ser simples uma inicial com 2 "botões" (botões representativos na interface) a de selecionar o sensor 1 (BMP), o sensor 2 (MPU) e na tela de cada sensor tem um botão de voltar que é pra voltar pra tela inicial e mostrando seus dados recebidos do json.

*especificação:*
	- _na tela dos sensores deve ter uma parte que mostra o stadodo sensor, e no codigo deve ser feita uma simples verificação se os dados são considerados normais no geral ou anormais, tipo uma temperatur ambiente ta a 100°C é meio anormal entt isso deve ser analisado de maneira simples dentro da próprio arquivo Fusion, considere que os sensores estarão em local ambiente normal
	- também em cada sensor vai ter um identificador de conexão, vou receber no json o status e ele dever ser mostrado de forma visual se o sensor 1 ta conectado ou não e se o sensor 2 ta conectado ou não_

---
# Detalhes da criação dos arquivos

***Divisão de arquivos do projeto:*** 
- **BspGpio** - lgpiod para a configuração de todos os pinos do projeto
- **BspButton** - responsável pela configuração dos pinos e funcionamento dos botões
- **BspScreen** -  utilização da lib GTX para criação da interface limpa
- **Fusion** - recebe os MQTTs dos outros nós 
- **Main** - integra tudo
- **Makefile** - compila tudo e gera os executáveis

---
```
cascata/
├── Makefile
├── inc/
│   ├── BspGpio.h
│   ├── BspButton.h
│   ├── BspScreen.h
│   └── Fusion.h
└── src/
    ├── BspGpio.c
    ├── BspButton.c
    ├── BspScreen.c
    ├── Fusion.c
    └── Main.c
```

---
# Pinagem física a ser considerada

| Pino no Display    | Função                            | Pino GPIO (RPi 5)           | Pino Físico na Placa (40-pin) |     |
| ------------------ | --------------------------------- | --------------------------- | ----------------------------- | --- |
| **D0**             | Bit de dados 0                    | **GPIO 16**                 | Pino 36                       |     |
| **D1**             | Bit de dados 1                    | **GPIO 17**                 | Pino 11                       |     |
| **D2**             | Bit de dados 2                    | **GPIO 18**                 | Pino 12                       |     |
| **D3**             | Bit de dados 3                    | **GPIO 19**                 | Pino 35                       |     |
| **D4**             | Bit de dados 4                    | **GPIO 20**                 | Pino 38                       |     |
| **D5**             | Bit de dados 5                    | **GPIO 21**                 | Pino 40                       |     |
| **D6**             | Bit de dados 6                    | **GPIO 22**                 | Pino 15                       |     |
| **D7**             | Bit de dados 7                    | **GPIO 23**                 | Pino 16                       |     |
| **LCD_RST**        | Reset do Display                  | GPIO 27                     | Pino 13                       |     |
| **LCD_CS**         | Chip Select (Habilita o display)  | GPIO 5                      | Pino 29                       |     |
| **LCD_RS** (ou DC) | Register Select (Comando / Dados) | GPIO 24                     | Pino 18                       |     |
| **LCD_WR**         | Write (Sinal de escrita)          | GPIO 25                     | Pino 22                       |     |
| **LCD_RD**         | Read (Sinal de leitura)           | GPIO 26 _(ou fixo em 3.3V)_ | Pino 37 _(ou Pino 1 / 17)_    |     |
| **GND**            | Referência de Terra (0V)          | GND                         | Pino 14, 34 ou 39             |     |

_Os botões consistem em um de ENTER e um de SELECÃO, um passa selecionando os botões na tela e o outro confirma.

| **Botão**          | Função     | **Pino GPIO Recomendado (RPi 5)** | **Pino Físico na Placa** | **Pino GND Adjacente** |
| ------------------ | ---------- | --------------------------------- | ------------------------ | ---------------------- |
| **Botão vermelho** | ENTER      | GPIO 4                            | **Pino 7**               | Pino 9 (GND)           |
| **Botão preto**    | SELECIONAR | GPIO 6                            | **Pino 31**              | Pino 30 (GND)          |

![[Pasted image 20260917165742.png|620]]

---

# Bibliotecas 
#### Lib gráfica GTK
```bash
sudo apt install libgtk-4-dev pkg-config build-essential
```
para a criação da interface do sistema
#### Lib para pinagem LGPIOD
```bash
sudo apt update && sudo apt install libgpiod-dev
```
para a configuração física da pinagem
#### Lib CJSON
```bash
sudo apt install -y libcjson-dev
```
para o tratamento e exibição do Json recebido

---
# Display

https://www.lcdwiki.com/2.4inch_Arduino_Display

| **Parâmetro**                | **Especificação Correta**                                                             |
| ---------------------------- | ------------------------------------------------------------------------------------- |
| **Driver IC**                | ILI9341                                                                               |
| **Tensão de Operação**       | 5V (Backlight) / 3.3V (Lógica)                                                        |
| **Tipo de Display**          | TFT LCD (2.4")                                                                        |
| **Resolução Nativa**         | 320 x 240 pixels                                                                      |
| **Interface de Comunicação** | **Paralela de 8 bits** (Barramento D0 a D7)                                           |
| **Sinais de Controle**       | LCD_WR, LCD_RD, LCD_RS, LCD_CS, LCD_RST                                               |
| **Orientação (Rotate)**      | 0 (Retrato), 1 (Paisagem - 90°), 2 (Retrato Invertido), 3 (Paisagem Invertida - 270°) |
| **Profundidade de Cor**      | RGB565 (16-bit / 65.536 cores)                                                        |

--- 
# Informações da tela

- **Janela:** 320x240 (paisagem), sem redimensionar, então não sai espelhada. Existe um `dSCREEN_FULLSCREEN` para ligar tela cheia depois.
- **Navegação:** na tela inicial, SELECIONAR alterna entre MPU e BMP (o selecionado ganha borda azul) e ENTER abre a tela. Nas telas dos sensores, o VOLTAR está sempre selecionado, então ENTER volta para a inicial.
- **Indicador de conexão:** `● ONLINE` (verde) ou `● OFFLINE` (vermelho), vindo do `Status` do JSON.
- **Estado:** `NORMAL` (verde), `ANORMAL` (vermelho), `DADO OBSOLETO` ou `SEM DADOS` (amarelo). Em dado obsoleto, o último valor continua na tela.
- **Teclado para teste:** `Espaço` = SELECIONAR e `Enter` = ENTER, para testar sem os botões físicos.
- **MPU:** a tela mostra os eixos x, y e z da aceleração, como no esboço.

---
# Atenção
- verifique a orientação da tela, deve ser horizontal e os pixels certos, cuidado pra não sair espelhado

 
```bash
//"pacote de ferramentas para verificação de GPIO"
sudo apt install gpiod 

//"verificação do status de cada pino"
gpioinfo gpiochip4
gpiodetect

```

---
# Status dos pinos e da conexão

```bash
caio@rpi5-caio:~ $ gpiodetect
gpiochip0 [pinctrl-rp1] (54 lines)
gpiochip10 [gpio-brcmstb@107d508500] (32 lines)
gpiochip11 [gpio-brcmstb@107d517c00] (15 lines)
gpiochip12 [gpio-brcmstb@107d517c20] (6 lines)
gpiochip13 [gpio-brcmstb@107d508520] (4 lines)


caio@rpi5-caio:~ $ gpioinfo -c gpiochip0
gpiochip0 - 54 lines:
	line   0:	"ID_SDA"        	input
	line   1:	"ID_SCL"        	input
	line   2:	"GPIO2"         	input consumer="kernel"
	line   3:	"GPIO3"         	input consumer="kernel"
	line   4:	"GPIO4"         	input
	line   5:	"GPIO5"         	input
	line   6:	"GPIO6"         	input
	line   7:	"GPIO7"         	input
	line   8:	"GPIO8"         	input
	line   9:	"GPIO9"         	input
	line  10:	"GPIO10"        	input
	line  11:	"GPIO11"        	input
	line  12:	"GPIO12"        	input
	line  13:	"GPIO13"        	input
	line  14:	"GPIO14"        	input
	line  15:	"GPIO15"        	input
	line  16:	"GPIO16"        	input
	line  17:	"GPIO17"        	input
	line  18:	"GPIO18"        	input
	line  19:	"GPIO19"        	input
	line  20:	"GPIO20"        	input
	line  21:	"GPIO21"        	input
	line  22:	"GPIO22"        	input
	line  23:	"GPIO23"        	input
	line  24:	"GPIO24"        	input
	line  25:	"GPIO25"        	input
	line  26:	"GPIO26"        	input
	line  27:	"GPIO27"        	input
	line  28:	"PCIE_RP1_WAKE" 	input
	line  29:	"FAN_TACH"      	input
	line  30:	"HOST_SDA"      	input
	line  31:	"HOST_SCL"      	input
	line  32:	"ETH_RST_N"     	output active-low consumer="phy-reset"
	line  33:	"-"             	input
	line  34:	"CD0_IO0_MICCLK"	output consumer="cam0_reg"
	line  35:	"CD0_IO0_MICDAT0"	input
	line  36:	"RP1_PCIE_CLKREQ_N"	input
	line  37:	"-"             	input
	line  38:	"CD0_SDA"       	input
	line  39:	"CD0_SCL"       	input
	line  40:	"CD1_SDA"       	input
	line  41:	"CD1_SCL"       	input
	line  42:	"USB_VBUS_EN"   	output consumer="kernel"
	line  43:	"USB_OC_N"      	input consumer="kernel"
	line  44:	"RP1_STAT_LED"  	output active-low consumer="PWR"
	line  45:	"FAN_PWM"       	output
	line  46:	"CD1_IO0_MICCLK"	output consumer="cam1_reg"
	line  47:	"2712_WAKE"     	input
	line  48:	"CD1_IO1_MICDAT1"	input
	line  49:	"EN_MAX_USB_CUR"	output
	line  50:	"-"             	input
	line  51:	"-"             	input
	line  52:	"-"             	input
	line  53:	"-"             	input

```

---
# interface limpa e simples
![[Pasted image 20261110154546.png]]

---
compilar e executar o projeto:

```bash
make clean && make ./build/cascata
```

---

