#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <lgpio.h>

#define PIN_RST 17
#define PIN_CS  27
#define PIN_RS  22
#define PIN_WR  23
#define PIN_RD  24

static int handle = -1;

// Escrita direta de pinos desembalada (sem overhead de loop)
static inline void write_byte(uint8_t val) {
    lgGpioWrite(handle, 5,  (val      ) & 1);
    lgGpioWrite(handle, 6,  (val >> 1 ) & 1);
    lgGpioWrite(handle, 12, (val >> 2 ) & 1);
    lgGpioWrite(handle, 13, (val >> 3 ) & 1);
    lgGpioWrite(handle, 16, (val >> 4 ) & 1);
    lgGpioWrite(handle, 19, (val >> 5 ) & 1);
    lgGpioWrite(handle, 20, (val >> 6 ) & 1);
    lgGpioWrite(handle, 21, (val >> 7 ) & 1);

    lgGpioWrite(handle, PIN_WR, 0);
    lgGpioWrite(handle, PIN_WR, 1);
}

static void write_cmd(uint8_t cmd) {
    lgGpioWrite(handle, PIN_RS, 0);
    lgGpioWrite(handle, PIN_CS, 0);
    write_byte(cmd);
    lgGpioWrite(handle, PIN_CS, 1);
}

static void write_data(uint8_t data) {
    lgGpioWrite(handle, PIN_RS, 1);
    lgGpioWrite(handle, PIN_CS, 0);
    write_byte(data);
    lgGpioWrite(handle, PIN_CS, 1);
}

void init_display() {
    printf("[DRIVER C] Conectando aos pinos GPIO do RPi 5...\n");
    fflush(stdout);

    for (int chip = 4; chip >= 0; chip--) {
        handle = lgGpiochipOpen(chip);
        if (handle >= 0) {
            printf("[DRIVER C] Sucesso: Chip GPIO %d aberto.\n", chip);
            fflush(stdout);
            break;
        }
    }

    if (handle < 0) {
        printf("[ERRO DRIVER C] Nao foi possivel abrir nenhum chip GPIO.\n");
        fflush(stdout);
        return;
    }

    lgGpioClaimOutput(handle, 0, PIN_RD, 1);
    lgGpioClaimOutput(handle, 0, PIN_WR, 1);
    lgGpioClaimOutput(handle, 0, PIN_RS, 1);
    lgGpioClaimOutput(handle, 0, PIN_RST, 1);
    lgGpioClaimOutput(handle, 0, PIN_CS, 1);

    int d_pins[8] = {5, 6, 12, 13, 16, 19, 20, 21};
    for (int i = 0; i < 8; i++) {
        lgGpioClaimOutput(handle, 0, d_pins[i], 0);
    }

    // Reset por Hardware
    lgGpioWrite(handle, PIN_RST, 1);
    usleep(10000);
    lgGpioWrite(handle, PIN_RST, 0);
    usleep(50000);
    lgGpioWrite(handle, PIN_RST, 1);
    usleep(150000);

    write_cmd(0x01); // Reset Software
    usleep(150000);

    // Sequência de Inicialização do ILI9341
    write_cmd(0xCB); write_data(0x39); write_data(0x2C); write_data(0x00); write_data(0x34); write_data(0x02);
    write_cmd(0xCF); write_data(0x00); write_data(0xC1); write_data(0x30);
    write_cmd(0xE8); write_data(0x85); write_data(0x00); write_data(0x78);
    write_cmd(0xEA); write_data(0x00); write_data(0x00);
    write_cmd(0xED); write_data(0x64); write_data(0x03); write_data(0x12); write_data(0x81);
    write_cmd(0xF7); write_data(0x20);

    write_cmd(0xC0); write_data(0x23);
    write_cmd(0xC1); write_data(0x10);
    write_cmd(0xC5); write_data(0x3E); write_data(0x28);
    write_cmd(0xC7); write_data(0x86);

    write_cmd(0x36); write_data(0x28); // Orientação
    write_cmd(0x3A); write_data(0x55); // Formato RGB565

    write_cmd(0xB1); write_data(0x00); write_data(0x18);
    write_cmd(0xB6); write_data(0x08); write_data(0x82); write_data(0x27);

    write_cmd(0x11); // Despertar
    usleep(120000);

    write_cmd(0x29); // Ligar imagem
    usleep(20000);

    printf("[DRIVER C] Display ILI9341 inicializado com sucesso!\n");
    fflush(stdout);
}

void send_frame(const uint8_t *rgb24_buffer) {
    if (handle < 0) return;

    write_cmd(0x2A);
    write_data(0x00); write_data(0x00); write_data(0x01); write_data(0x3F);

    write_cmd(0x2B);
    write_data(0x00); write_data(0x00); write_data(0x00); write_data(0xEF);

    write_cmd(0x2C);

    lgGpioWrite(handle, PIN_RS, 1);
    lgGpioWrite(handle, PIN_CS, 0);

    for (int i = 0; i < 320 * 240 * 3; i += 3) {
        uint8_t r = rgb24_buffer[i];
        uint8_t g = rgb24_buffer[i + 1];
        uint8_t b = rgb24_buffer[i + 2];

        uint8_t b1 = (r & 0xF8) | ((g & 0xE0) >> 5);
        uint8_t b2 = ((g & 0x1C) << 3) | (b >> 3);

        write_byte(b1);
        write_byte(b2);
    }

    lgGpioWrite(handle, PIN_CS, 1);
}