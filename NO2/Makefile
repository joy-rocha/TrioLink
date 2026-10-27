# =====================================================================
# Makefile - No 2 (MPU6500 + MQTT)
#
# Estrutura esperada:
#   Inc/   todos os .h  (AssertTypes.h, BmpTracker.h, IioSysfs.h,
#                         Mpu6500Iio.h, MpuPayloads.h, MqttWrapper.h)
#   Src/   todos os .c  (BmpTracker.c, IioSysfs.c, Main.c, Mpu6500Iio.c,
#                         MpuPayloads.c, MqttWrapper.c)
#   build/ criado automaticamente, com os .o e .d (apagado por "make clean")
#
# Uso:
#   make              compila ./no2
#   make run ARGS=IP  compila (se preciso) e roda: ./no2 IP
#   make clean        remove build/ e o executavel
#
# Dependencias (Raspberry Pi OS / Debian):
#   sudo apt install build-essential libmosquitto-dev libcjson-dev
# =====================================================================

CC       ?= gcc
TARGET   := no2

INC_DIR  := Inc
SRC_DIR  := Src
BUILD_DIR:= build

SRCS     := $(wildcard $(SRC_DIR)/*.c)
OBJS     := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS     := $(OBJS:.o=.d)

# -I$(INC_DIR)  : onde o compilador procura os .h deste projeto
# -MMD -MP      : gera os .d, para o make recompilar sozinho quando um
#                 .h incluido mudar (nao so quando o .c muda)
# -Wno-comment  : o estilo de cabecalho de funcao das normas Assert coloca
#                 um "/**" dentro de um comentario ja aberto; o aviso do
#                 GCC para isso e inofensivo, entao fica silenciado aqui
CFLAGS   := -std=gnu11 -Wall -Wextra -Wno-comment -g -I$(INC_DIR) -MMD -MP

# -lmosquitto : cliente MQTT (MqttWrapper)
# -lcjson     : montagem dos payloads JSON (MpuPayloads, BmpTracker)
# -lpthread   : mutex interno (MqttWrapper, BmpTracker)
# -lm         : round()/pow()/fabs() (MpuPayloads, Mpu6500Iio)
LDLIBS   := -lmosquitto -lcjson -lpthread -lm

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

run: $(TARGET)
	./$(TARGET) $(ARGS)

clean:
	rm -rf $(BUILD_DIR) $(TARGET)

-include $(DEPS)
