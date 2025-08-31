CC=gcc
CFLAGS=-O2 -I/opt/homebrew/opt/raylib/include
LDFLAGS=-L/opt/homebrew/opt/raylib/lib -lraylib -lpthread -lm \
  -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio

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