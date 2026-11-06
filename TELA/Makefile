.RECIPEPREFIX = >
CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -pthread $(shell pkg-config --cflags freetype2)
LIBS    = -llgpio -lfreetype -lcjson -lm -pthread
SRC     = Main.c Ui.c Gfx.c Lcd.c Buttons.c DecodeBmp.c DecodeMpu.c

monitor: $(SRC) $(wildcard *.h)
> $(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean:
> rm -f monitor