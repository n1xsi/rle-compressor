CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2
TARGET = rle
SRC = src/main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.rle
