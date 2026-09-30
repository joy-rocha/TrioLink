/* Driver Paralelo 8-bits ILI9341 para Raspberry Pi 5 usando lgpio */
#include "lcd.h"
#include <lgpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define GPIOCHIP      0

/* Mapeamento de Pinos Controle */
#define PIN_DC        24    /* LCD_RS (Command/Data) */
#define PIN_RST       25    /* LCD_RST */
#define PIN_CS        8     /* LCD_CS */
#define PIN_WR        23    /* LCD_WR */

/* Pinos de Dados (D0 ate D7) */
static const int pins_data[8] = {12, 13, 16, 19, 20, 21, 26, 27};

static int gh = -1;

static inline void write8(uint8_t d) {
    for (int i = 0; i < 8; i++) {
        lgGpioWrite(gh, pins_data[i], (d >> i) & 1);
    }
    lgGpioWrite(gh, PIN_WR, 0);
    lgGpioWrite(gh, PIN_WR, 1);
}

static void cmd(uint8_t c) {
    lgGpioWrite(gh, PIN_DC, 0);
    lgGpioWrite(gh, PIN_CS, 0);
    write8(c);
    lgGpioWrite(gh, PIN_CS, 1);
}

static void data_byte(uint8_t d) {
    lgGpioWrite(gh, PIN_DC, 1);
    lgGpioWrite(gh, PIN_CS, 0);
    write8(d);
    lgGpioWrite(gh, PIN_CS, 1);
}

static void cmdv(uint8_t c, const uint8_t *args, int n) {
    cmd(c);
    for (int i = 0; i < n; i++) {
        data_byte(args[i]);
    }
}

#define CMD(c, ...) do { const uint8_t a_[] = { __VA_ARGS__ }; cmdv((c), a_, (int)sizeof a_); } while (0)

int lcd_init(void) {
    gh = lgGpiochipOpen(GPIOCHIP);
    if (gh < 0) { fprintf(stderr, "lgGpiochipOpen falhou (%d)\n", gh); return -1; }

    lgGpioClaimOutput(gh, 0, PIN_DC, 0);
    lgGpioClaimOutput(gh, 0, PIN_RST, 1);
    lgGpioClaimOutput(gh, 0, PIN_CS, 1);
    lgGpioClaimOutput(gh, 0, PIN_WR, 1);

    for (int i = 0; i < 8; i++) {
        lgGpioClaimOutput(gh, 0, pins_data[i], 0);
    }

    /* Reset por Hardware */
    lgGpioWrite(gh, PIN_RST, 1); usleep(10000);
    lgGpioWrite(gh, PIN_RST, 0); usleep(20000);
    lgGpioWrite(gh, PIN_RST, 1); usleep(120000);

    /* Inicializacao do Controlador ILI9341 */
    CMD(0xEF, 0x03, 0x80, 0x02);
    CMD(0xCF, 0x00, 0xC1, 0x30);
    CMD(0xED, 0x64, 0x03, 0x12, 0x81);
    CMD(0xE8, 0x85, 0x00, 0x78);
    CMD(0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02);
    CMD(0xF7, 0x20);
    CMD(0xEA, 0x00, 0x00);
    CMD(0xC0, 0x23);
    CMD(0xC1, 0x10);
    CMD(0xC5, 0x3E, 0x28);
    CMD(0xC7, 0x86);
    CMD(0x36, 0x28);                 /* Orientacao Paisagem */
    CMD(0x3A, 0x55);                 /* RGB565 (16 bits) */
    CMD(0xB1, 0x00, 0x18);
    CMD(0xB6, 0x08, 0x82, 0x27);
    CMD(0xF2, 0x00);
    CMD(0x26, 0x01);
    CMD(0xE0, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00);
    CMD(0xE1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F);
    cmd(0x11); usleep(120000);       /* Exit Sleep */
    cmd(0x29); usleep(20000);        /* Display ON */
    return 0;
}

void lcd_flush(const uint16_t *fb) {
    uint8_t a[4];
    a[0] = 0; a[1] = 0; a[2] = (LCD_W - 1) >> 8; a[3] = (LCD_W - 1) & 0xFF; cmdv(0x2A, a, 4);
    a[0] = 0; a[1] = 0; a[2] = (LCD_H - 1) >> 8; a[3] = (LCD_H - 1) & 0xFF; cmdv(0x2B, a, 4);
    cmd(0x2C);

    lgGpioWrite(gh, PIN_DC, 1);
    lgGpioWrite(gh, PIN_CS, 0);

    const size_t total = (size_t)LCD_W * LCD_H;
    for (size_t i = 0; i < total; i++) {
        uint16_t color = fb[i];
        write8(color >> 8);   /* Byte Alto */
        write8(color & 0xFF); /* Byte Baixo */
    }

    lgGpioWrite(gh, PIN_CS, 1);
}

void lcd_close(void) {
    if (gh >= 0) lgGpiochipClose(gh);
    gh = -1;
}

/* Stubs para o touch (desativado até ter o módulo ADC) */
int touch_init(void) { return 0; }
bool touch_read_raw(int *rx, int *ry) { (void)rx; (void)ry; return false; }
bool touch_read(int *x, int *y) { (void)x; (void)y; return false; }