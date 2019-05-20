CC=gcc
CFLAGS=-std=c11 -Wall
LDFLAGS=-O3 -lX11 -lGL
OBJ=$(patsubst %.c, %.o, $(wildcard *.c))
TARGET=xclone
.PHONY: all clean
all: $(TARGET)
$(TARGET): $(OBJ)
	$(CC) $(LDFLAGS) $(OBJ) -o $(TARGET)
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
clean:
	rm -f $(OBJ) $(TARGET)
