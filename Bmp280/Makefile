# ==============================================================
# Makefile - No BMP280 (TrioLink, Desafio 1)
# ==============================================================
# Compila BspSensorBmp280 + SensorBmp280 + SimpleMovingAvg + JsonBmp280 +
# MqttWrapper + main.c num unico executavel.
#
# Estrutura esperada do projeto:
#   Makefile
#   Inc/    -> todos os .h
#   Src/    -> todos os .c
#   build/  -> criada pelo make: guarda os .o e .d (pode apagar a vontade)
#
# Uso:
#   make          -> compila (so recompila o que mudou)
#   make run      -> compila e executa
#   make clean    -> apaga a pasta build e o executavel
#   make rebuild  -> clean + compila tudo de novo do zero
# ==============================================================

# Nome do executavel final
TARGET := bmp280

# Pastas do projeto
SRC_DIR := Src
INC_DIR := Inc
BUILD_DIR := build

# Compilador
CC := gcc

# Flags de compilacao:
#   -std=gnu11    : padrao C11 com extensoes GNU (precisa de clock_gettime etc)
#   -Wall -Wextra : liga os avisos do compilador (ajuda a achar bug cedo)
#   -I$(INC_DIR)  : procura os .h na pasta Inc
#   -MMD -MP      : gera arquivos .d de dependencia (explicado mais abaixo)
CFLAGS := -std=gnu11 -Wall -Wextra -I$(INC_DIR) -MMD -MP

# Flags de linkagem (bibliotecas externas):
#   -lm         : biblioteca matematica (powf em SensorBmp280.c)
#   -lcjson     : biblioteca cJSON (JsonBmp280.c)
#   -lmosquitto : biblioteca MQTT (MqttWrapper.c)
#   -lpthread   : mutex interno do MqttWrapper (a thread de rede em si e da
#                 propria libmosquitto, via mosquitto_loop_start)
LDFLAGS := -lm -lcjson -lmosquitto -lpthread

# Lista de todos os arquivos .c do projeto (nomes, sem a pasta)
SRCS := main_no1.c \
        BspSensorBmp280.c \
        SensorBmp280.c \
        SimpleMovingAvg.c \
        JsonBmp280.c \
        MqttWrapper.c

# Gera a lista de .o dentro da pasta build (main.c -> build/main.o, etc)
OBJS := $(addprefix $(BUILD_DIR)/,$(SRCS:.c=.o))

# Gera a lista de .d (dependencias) a partir da lista de .o
DEPS := $(OBJS:.o=.d)

# ------------------------------------------------------------------
# Regra padrao (a primeira do arquivo = a que roda com "make" sem args)
# ------------------------------------------------------------------
all: $(TARGET)

# Linkagem: junta todos os .o num executavel.
# $@ = nome do alvo (bmp280); $^ = todos os pre-requisitos (todos os .o)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Regra generica: como gerar build/X.o a partir de Src/X.c.
# $< = o primeiro pre-requisito (o .c); -c = compila sem linkar.
# "| $(BUILD_DIR)" = a pasta precisa existir, mas nao forca recompilacao.
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

# Cria a pasta build se ela ainda nao existir
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Inclui os .d gerados (se existirem), para o make saber que, por exemplo,
# SensorBmp280.o tambem depende de SensorBmp280.h e AssertTypes.h -- editar
# um .h faz o make recompilar os .c certos, nao so quando o .c muda.
-include $(DEPS)

# ------------------------------------------------------------------
# Alvos auxiliares (nao geram arquivo com esse nome, entao .PHONY)
# ------------------------------------------------------------------
.PHONY: all run clean rebuild

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

rebuild: clean all
