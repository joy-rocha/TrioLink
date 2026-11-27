# Makefile do projeto cascata (no de display)
#
#   make         compila e gera build/cascata
#   make run     compila e executa
#   make clean   remove a pasta build

TARGET      := cascata
SRC_DIR     := src
INC_DIR     := inc
BUILD_DIR   := build

CC          ?= gcc
PKGS        := gtk4 libgpiod libcjson libmosquitto

CFLAGS      := -std=gnu11 -Wall -Wextra -O2 -I$(INC_DIR) -MMD -MP \
               $(shell pkg-config --cflags $(PKGS))
LDLIBS      := $(shell pkg-config --libs $(PKGS)) -lpthread

SRCS        := $(wildcard $(SRC_DIR)/*.c)
OBJS        := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS        := $(OBJS:.o=.d)

.PHONY: all run clean

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDLIBS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	mkdir -p $@

run: all
	./$(BUILD_DIR)/$(TARGET)

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)