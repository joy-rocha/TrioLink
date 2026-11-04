# ==============================================================
# Makefile - No BMP280 (TrioLink, Desafio 1)
# ==============================================================
# Compila BspSensorBmp280 + SensorBmp280 + SimpleMovingAvg + JsonBmp280 +
# PublisherBmp280 + main.c num unico executavel.
#
# Uso:
#   make          -> compila (so recompila o que mudou)
#   make run      -> compila e executa
#   make clean    -> apaga os .o e o executavel
#   make rebuild  -> clean + compila tudo de novo do zero
# ==============================================================

# Nome do executavel final
TARGET := no_bmp280

# Compilador
CC := gcc

# Flags de compilacao:
#   -std=gnu11    : padrao C11 com extensoes GNU (precisa de clock_gettime etc)
#   -Wall -Wextra : liga os avisos do compilador (ajuda a achar bug cedo)
#   -I.           : procura os .h na pasta atual
#   -MMD -MP      : gera arquivos .d de dependencia (explicado mais abaixo)
CFLAGS := -std=gnu11 -Wall -Wextra -I. -MMD -MP

# Flags de linkagem (bibliotecas externas):
#   -lm         : biblioteca matematica (powf em SensorBmp280.c)
#   -lcjson     : biblioteca cJSON (JsonBmp280.c)
#   -lmosquitto : biblioteca MQTT (MqttWrapper.c)
#   -lpthread   : mutex interno do MqttWrapper (a thread de rede em si e da
#                 propria libmosquitto, via mosquitto_loop_start)
LDFLAGS := -lm -lcjson -lmosquitto -lpthread

# Lista de todos os arquivos .c do projeto
SRCS := main.c \
        BspSensorBmp280.c \
        SensorBmp280.c \
        SimpleMovingAvg.c \
        JsonBmp280.c \
        PublisherBmp280.c \
        MqttWrapper.c

# Gera a lista de .o a partir da lista de .c (main.c -> main.o, etc)
OBJS := $(SRCS:.c=.o)

# Gera a lista de .d (dependencias) a partir da lista de .o
DEPS := $(OBJS:.o=.d)

# ------------------------------------------------------------------
# Regra padrao (a primeira do arquivo = a que roda com "make" sem args)
# ------------------------------------------------------------------
all: $(TARGET)

# Linkagem: junta todos os .o num executavel.
# $@ = nome do alvo (no_bmp280); $^ = todos os pre-requisitos (todos os .o)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Regra generica: como gerar um .o a partir do .c de mesmo nome.
# $< = o primeiro pre-requisito (o .c); -c = compila sem linkar.
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

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
	rm -f $(OBJS) $(DEPS) $(TARGET)

rebuild: clean all
