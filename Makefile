CC=gcc
CFLAGS=-O2 -Wall -Wextra -std=c99
LDFLAGS=-lraylib -lGL -lm -lpthread -ldl -lrt -lX11

SRC=$(wildcard *.c)
OBJ=$(patsubst %.c, %.o, $(SRC))

TARGET=StegaNet

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(CFLAGS) $(LDFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f $(TARGET) $(OBJ) stego_*.png stego_*.wav
	rm -rf inbox
	rm -rf random*
	rm -rf encoded*

install-deps:
	sudo apt-get update
	sudo apt-get install -y libasound2-dev libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev libxcursor-dev libxinerama-dev

.PHONY: all clean install-deps