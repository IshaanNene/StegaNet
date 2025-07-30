CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_POSIX_C_SOURCE=200809L
LIBS = -lraylib -lpthread -lm

# macOS specific flags
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    LIBS += -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreAudio
endif

# Source files
SOURCES = socket_chat.c bigl.c
HEADERS = raygui.h
TARGET = steganet_enhanced

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)

clean:
	rm -f $(TARGET)
	rm -f *.o
	rm -f randomImage.png
	rm -f encoded_image.png
	rm -f randomAudio.wav
	rm -f encoded_audio.wav
	rm -f youtube_audio.wav

install-deps:
	@echo "Installing dependencies for macOS..."
	@if ! command -v brew &> /dev/null; then \
		echo "Homebrew not found. Please install Homebrew first."; \
		echo "Visit: https://brew.sh"; \
		exit 1; \
	fi
	brew install raylib

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

help:
	@echo "Available targets:"
	@echo "  all          - Build the application (default)"
	@echo "  clean        - Remove build files and generated files"
	@echo "  install-deps - Install raylib using Homebrew (macOS)"
	@echo "  run          - Build and run the application"
	@echo "  debug        - Build with debug symbols"
	@echo "  help         - Show this help message"

.PHONY: all clean install-deps run debug help