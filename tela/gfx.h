#ifndef GFX_H
#define GFX_H
#include <stdint.h>
#include "lcd.h"

extern uint16_t fb[LCD_W * LCD_H];

enum { ALIGN_LEFT = 0, ALIGN_CENTER = 1, ALIGN_RIGHT = 2 };

static inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

int  gfx_init(const char *font_path);
void gfx_clear(uint16_t c);
void gfx_fill_rect(int x, int y, int w, int h, uint16_t c);
void gfx_fill_rrect(int x, int y, int w, int h, int r, uint16_t c);
void gfx_fill_circle(int cx, int cy, int r, uint16_t c);
void gfx_ring(int cx, int cy, int rx, int ry, int thick, uint16_t c);
/* x = âncora horizontal, yc = centro vertical do texto, px = altura da fonte, texto em UTF-8 */
void gfx_text(int x, int yc, int px, const char *utf8, uint16_t color, int align);
int  gfx_text_width(const char *utf8, int px);

#endif
