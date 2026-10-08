# =====================================================================
# Makefile - teste da MqttWrapper
#
# Uso:
#   make          compila o executavel "mqtt_test"
#   make run      compila (se preciso) e executa
#   make clean    remove objetos e executavel
#
# Requisitos: gcc, make, libmosquitto-dev
#   sudo apt install build-essential libmosquitto-dev
# =====================================================================

CC      ?= gcc
TARGET  := mqtt_test

SRCS    := Main.c MqttWrapper.c
OBJS    := $(SRCS:.c=.o)
DEPS    := $(SRCS:.c=.d)

# -Wall -Wextra : avisos uteis do compilador
# -MMD -MP      : gera arquivos .d para o make recompilar quando um .h mudar
# -Wno-comment  : o estilo de cabecalho de funcao das normas Assert coloca um
#                 "/**" dentro de um bloco de comentario ja aberto, o que o
#                 GCC reporta como aviso. E inofensivo, entao e silenciado.
CFLAGS  := -std=gnu11 -Wall -Wextra -Wno-comment -g -I. -MMD -MP

# -lmosquitto : biblioteca cliente MQTT
# -lpthread   : mutex usado internamente pela wrapper
LDLIBS  := -lmosquitto -lpthread

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)

-include $(DEPS)
