/* Driver Paralelo 8-bits ILI9341 para Raspberry Pi 5
   - Caminho rápido: escreve direto no registrador OUT do RP1 via /dev/gpiomem0
     (2 escritas por byte) -> o quadro inteiro é enviado em poucas dezenas de ms.
   - Fallback automático: se /dev/gpiomem0 não estiver acessível, usa lgpio (grupo).
   - Só reenvia o retângulo que mudou desde o último quadro. */
#include "lcd.h"
#include <lgpio.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define GPIOCHIP      0

/* Mapeamento de Pinos Controle */
#define PIN_DC        24    /* LCD_RS (Command/Data) */
#define PIN_RST       25    /* LCD_RST */
#define PIN_CS        8     /* LCD_CS */
#define PIN_WR        23    /* LCD_WR */

/* Pinos de Dados (D0 ate D7) */
static const int pins_data[8] = {12, 13, 16, 19, 20, 21, 26, 27};

/* ---------- Estado ---------- */
static int gh = -1;

/* Caminho lgpio (fallback): grupo D0..D7 (bits 0..7) + WR (bit 8) */
static const int grp_pins[9] = {12, 13, 16, 19, 20, 21, 26, 27, PIN_WR};
#define GRP_MASK  0x1FFULL
#define GRP_WR    0x100ULL
static int grp = -1;

/* Caminho rápido: registradores RIO do RP1 */
#define GPIOMEM_DEV   "/dev/gpiomem0"
#define GPIOMEM_SIZE  0x30000
#define RIO_OFFSET    0x10000     /* RIO bank0: OUT=+0x0, OE=+0x4, IN=+0x8 */
static volatile uint32_t *rio = NULL;
static void *map_base = NULL;
static uint32_t lut[256];         /* byte -> máscara dos pinos de dados */
static uint32_t ALLMASK;          /* todos os pinos do LCD */

static uint16_t prev[LCD_W * LCD_H];       /* último quadro enviado */
static bool have_prev = false;

/* ---------- Escrita de 1 byte ---------- */
static inline void write8(uint8_t d) {
    lgGroupWrite(gh, grp, (uint64_t)d,          GRP_MASK);   /* dado + WR = 0 */
    lgGroupWrite(gh, grp, (uint64_t)d | GRP_WR, GRP_MASK);   /* dado + WR = 1 */
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
    for (int i = 0; i < n; i++) data_byte(args[i]);
}

#define CMD(c, ...) do { const uint8_t a_[] = { __VA_ARGS__ }; cmdv((c), a_, (int)sizeof a_); } while (0)

/* Tenta habilitar o caminho rápido. Retorna true se funcionou. */
static bool fast_init(void) {
    int fd = open(GPIOMEM_DEV, O_RDWR | O_SYNC);
    if (fd < 0) return false;
    void *m = mmap(NULL, GPIOMEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (m == MAP_FAILED) return false;

    volatile uint32_t *r = (volatile uint32_t *)((char *)m + RIO_OFFSET);

    ALLMASK = (1u << PIN_DC) | (1u << PIN_RST) | (1u << PIN_CS) | (1u << PIN_WR);
    for (int i = 0; i < 8; i++) ALLMASK |= 1u << pins_data[i];

    /* Verificação de segurança: os pinos já foram reivindicados como saída pelo lgpio,
       então o registrador OE tem que mostrar todos eles. Se não, o offset está errado. */
    if ((r[1] & ALLMASK) != ALLMASK) { munmap(m, GPIOMEM_SIZE); return false; }

    for (int b = 0; b < 256; b++) {
        uint32_t v = 0;
        for (int i = 0; i < 8; i++) if ((b >> i) & 1) v |= 1u << pins_data[i];
        lut[b] = v;
    }
    map_base = m;
    rio = r;
    return true;
}

int lcd_init(void) {
    gh = lgGpiochipOpen(GPIOCHIP);
    if (gh < 0) { fprintf(stderr, "lgGpiochipOpen falhou (%d)\n", gh); return -1; }

    lgGpioClaimOutput(gh, 0, PIN_DC, 0);
    lgGpioClaimOutput(gh, 0, PIN_RST, 1);
    lgGpioClaimOutput(gh, 0, PIN_CS, 1);

    /* D0..D7 + WR como grupo (WR começa em 1); também serve de reivindicação dos pinos */
    const int levels[9] = {0, 0, 0, 0, 0, 0, 0, 0, 1};
    if (lgGroupClaimOutput(gh, 0, 9, grp_pins, levels) < 0) {
        fprintf(stderr, "lgGroupClaimOutput falhou\n");
        return -1;
    }
    grp = grp_pins[0];

    if (fast_init()) fprintf(stderr, "LCD: modo rapido (registradores RP1)\n");
    else             fprintf(stderr, "LCD: modo lgpio (sem acesso a %s)\n", GPIOMEM_DEV);

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

    have_prev = false;               /* primeiro flush envia tudo */
    return 0;
}

/* Envia os pixels da janela já configurada */
static void push_pixels(const uint16_t *fb, int x0, int y0, int x1, int y1) {
    lgGpioWrite(gh, PIN_DC, 1);
    lgGpioWrite(gh, PIN_CS, 0);

    if (rio) {
        const uint32_t WR = 1u << PIN_WR;
        uint32_t base = (rio[0] & ~ALLMASK) | (1u << PIN_DC) | (1u << PIN_RST);  /* CS = 0 */
        for (int y = y0; y <= y1; y++) {
            const uint16_t *row = fb + y * LCD_W;
            for (int x = x0; x <= x1; x++) {
                uint16_t c = row[x];
                uint32_t v = base | lut[c >> 8];
                rio[0] = v;        /* WR = 0 */
                rio[0] = v | WR;   /* WR = 1 (latch) */
                v = base | lut[c & 0xFF];
                rio[0] = v;
                rio[0] = v | WR;
            }
        }
    } else {
        for (int y = y0; y <= y1; y++) {
            const uint16_t *row = fb + y * LCD_W;
            for (int x = x0; x <= x1; x++) {
                uint16_t c = row[x];
                write8(c >> 8);
                write8(c & 0xFF);
            }
        }
    }

    lgGpioWrite(gh, PIN_CS, 1);
}

/* Envia só o retângulo que mudou desde o último quadro */
void lcd_flush(const uint16_t *fb) {
    int x0 = 0, y0 = 0, x1 = LCD_W - 1, y1 = LCD_H - 1;

    if (have_prev) {
        x0 = LCD_W; y0 = LCD_H; x1 = -1; y1 = -1;
        for (int y = 0; y < LCD_H; y++) {
            const uint16_t *a = fb   + y * LCD_W;
            const uint16_t *b = prev + y * LCD_W;
            if (memcmp(a, b, LCD_W * sizeof(uint16_t)) == 0) continue;
            if (y < y0) y0 = y;
            y1 = y;
            for (int x = 0; x < LCD_W; x++)
                if (a[x] != b[x]) { if (x < x0) x0 = x; break; }
            for (int x = LCD_W - 1; x >= 0; x--)
                if (a[x] != b[x]) { if (x > x1) x1 = x; break; }
        }
        if (x1 < 0) return;          /* nada mudou */
    }

    memcpy(prev, fb, sizeof prev);
    have_prev = true;

    uint8_t a[4];
    a[0] = x0 >> 8; a[1] = x0 & 0xFF; a[2] = x1 >> 8; a[3] = x1 & 0xFF; cmdv(0x2A, a, 4);
    a[0] = y0 >> 8; a[1] = y0 & 0xFF; a[2] = y1 >> 8; a[3] = y1 & 0xFF; cmdv(0x2B, a, 4);
    cmd(0x2C);

    push_pixels(fb, x0, y0, x1, y1);
}

void lcd_close(void) {
    if (map_base) { munmap(map_base, GPIOMEM_SIZE); map_base = NULL; rio = NULL; }
    if (gh >= 0) lgGpiochipClose(gh);
    gh = -1;
    grp = -1;
}

/* Stubs para o touch (desativado até ter o módulo ADC) */
int touch_init(void) { return 0; }
bool touch_read_raw(int *rx, int *ry) { (void)rx; (void)ry; return false; }
bool touch_read(int *x, int *y) { (void)x; (void)y; return false; }
