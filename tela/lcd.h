#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H
#include <stdint.h>
#include <stdbool.h>

#define LCD_W 320
#define LCD_H 240

/* Display ILI9341 (SPI) */
int  lcd_init(void);
void lcd_close(void);
void lcd_flush(const uint16_t *fb);          /* envia o framebuffer RGB565 inteiro */

/* Touch XPT2046 (SPI) - chamar depois de lcd_init() */
int  touch_init(void);
bool touch_read_raw(int *rx, int *ry);       /* valores brutos 0..4095 */
bool touch_read(int *x, int *y);             /* true se tocado; x,y em pixels */

#endif
