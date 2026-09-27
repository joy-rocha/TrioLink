CC = gcc

CFLAGS = -Wall -Wextra -IInc

TARGET = test_mpu

SRC = test_mpu.c \
      Src/mpu_iio.c \
      Src/iio_sysfs.c

OBJ = $(SRC:.c=.o)


$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)


%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -f $(OBJ) $(TARGET)