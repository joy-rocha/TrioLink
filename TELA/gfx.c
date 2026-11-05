/* Primitivas gráficas em framebuffer RGB565 + texto TrueType via FreeType */
#include "gfx.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <math.h>
#include <stdio.h>

uint16_t fb[LCD_W * LCD_H];

static FT_Library ft;
static FT_Face face;
static int cur_px = 0;

int gfx_init(const char *font_path) {
    if (FT_Init_FreeType(&ft)) return -1;
    if (FT_New_Face(ft, font_path, 0, &face)) {
        fprintf(stderr, "Fonte nao encontrada: %s\n", font_path);
        return -1;
    }
    return 0;
}

static void set_size(int px) {
    if (px != cur_px) { FT_Set_Pixel_Sizes(face, 0, px); cur_px = px; }
}

static inline void put_px(int x, int y, uint16_t c) {
    if ((unsigned)x < LCD_W && (unsigned)y < LCD_H) fb[y * LCD_W + x] = c;
}

static inline void blend_px(int x, int y, uint16_t c, int a) {
    if ((unsigned)x >= LCD_W || (unsigned)y >= LCD_H) return;
    uint16_t d = fb[y * LCD_W + x];
    int dr = (d >> 11) & 31, dg = (d >> 5) & 63, db = d & 31;
    int sr = (c >> 11) & 31, sg = (c >> 5) & 63, sb = c & 31;
    dr = (dr * (255 - a) + sr * a) / 255;
    dg = (dg * (255 - a) + sg * a) / 255;
    db = (db * (255 - a) + sb * a) / 255;
    fb[y * LCD_W + x] = (uint16_t)((dr << 11) | (dg << 5) | db);
}

void gfx_clear(uint16_t c) {
    for (int i = 0; i < LCD_W * LCD_H; i++) fb[i] = c;
}

void gfx_fill_rect(int x, int y, int w, int h, uint16_t c) {
    for (int j = y; j < y + h; j++)
        for (int i = x; i < x + w; i++) put_px(i, j, c);
}

void gfx_fill_rrect(int x, int y, int w, int h, int r, uint16_t c) {
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    for (int j = 0; j < h; j++) {
        int inset = 0;
        float dy = -1;
        if (j < r) dy = r - j - 0.5f;
        else if (j >= h - r) dy = j - (h - r) + 0.5f;
        if (dy >= 0) inset = (int)ceilf(r - sqrtf((float)r * r - dy * dy));
        for (int i = inset; i < w - inset; i++) put_px(x + i, y + j, c);
    }
}

void gfx_fill_circle(int cx, int cy, int r, uint16_t c) {
    for (int j = -r; j <= r; j++)
        for (int i = -r; i <= r; i++)
            if (i * i + j * j <= r * r) put_px(cx + i, cy + j, c);
}

void gfx_ring(int cx, int cy, int rx, int ry, int thick, uint16_t c) {
    float m = (float)(rx < ry ? rx : ry);
    for (int j = -ry - thick; j <= ry + thick; j++)
        for (int i = -rx - thick; i <= rx + thick; i++) {
            float d = hypotf((float)i / rx, (float)j / ry);
            if (fabsf(d - 1.0f) * m <= thick / 2.0f) put_px(cx + i, cy + j, c);
        }
}

static int utf8_next(const char **s) {
    const unsigned char *p = (const unsigned char *)*s;
    int c = *p;
    if (c < 0x80) { *s += 1; return c; }
    if ((c & 0xE0) == 0xC0 && p[1]) { *s += 2; return ((c & 0x1F) << 6) | (p[1] & 0x3F); }
    if ((c & 0xF0) == 0xE0 && p[1] && p[2]) { *s += 3; return ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F); }
    *s += 1;
    return '?';
}

int gfx_text_width(const char *s, int px) {
    set_size(px);
    int w = 0;
    while (*s) {
        int cp = utf8_next(&s);
        if (FT_Load_Char(face, cp, FT_LOAD_DEFAULT)) continue;
        w += (int)(face->glyph->advance.x >> 6);
    }
    return w;
}

void gfx_text(int x, int yc, int px, const char *s, uint16_t color, int align) {
    set_size(px);
    int w = gfx_text_width(s, px);
    int pen = (align == ALIGN_CENTER) ? x - w / 2 : (align == ALIGN_RIGHT) ? x - w : x;
    int base = yc + px * 36 / 100;           /* centraliza letras maiúsculas em yc */
    while (*s) {
        int cp = utf8_next(&s);
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER)) continue;
        FT_GlyphSlot g = face->glyph;
        for (unsigned r = 0; r < g->bitmap.rows; r++)
            for (unsigned c = 0; c < g->bitmap.width; c++) {
                int a = g->bitmap.buffer[r * g->bitmap.pitch + c];
                if (a) blend_px(pen + g->bitmap_left + (int)c, base - g->bitmap_top + (int)r, color, a);
            }
        pen += (int)(g->advance.x >> 6);
    }
}
