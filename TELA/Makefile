.RECIPEPREFIX = >
CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -pthread $(shell pkg-config --cflags freetype2)
LIBS    = -llgpio -lfreetype -lcjson -lm -pthread
SRC     = main.c ui.c gfx.c lcd.c buttons.c DecodeBMP.c DecodeMPU.c

monitor: $(SRC) $(wildcard *.h)
> $(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

clean:
> rm -f monitor