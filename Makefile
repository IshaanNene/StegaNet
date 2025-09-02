CC = gcc
CFLAGS = -O2

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
    RAYLIB_PATH =
endif
ifeq ($(UNAME_S),Darwin)
    LIBS = -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm -lpthread
    RAYLIB_PATH = -I/opt/homebrew/opt/raylib/include -L/opt/homebrew/opt/raylib/lib
endif

OBJDIR = obj
BINDIR = bin

SOURCES = main.c network.c steganography.c utils.c ui.c
OBJECTS = $(SOURCES:%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/stegachat

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(RAYLIB_PATH) $(LIBS)

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) $(RAYLIB_PATH) -I. -c $< -o $@

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	rm -f *.png *.wav temp_encoded_* encoded_* received_*
	clear

install-deps:
	sudo apt update
	sudo apt install libraylib-dev

install-deps-mac:
	brew install raylib

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET)

run: $(TARGET)
	cd $(BINDIR) && ./stegachat

update:
	git status
	git add .
	git commit -m "Update"
	git push

help:
	@echo "Available targets:"
	@echo "  all          - Build the application"
	@echo "  clean        - Remove build files and generated media"
	@echo "  debug        - Build with debug symbols"
	@echo "  run          - Build and run the application"
	@echo "  install-deps - Install dependencies (Ubuntu/Debian)"
	@echo "  install-deps-mac - Install dependencies (macOS)"
	@echo "  help         - Show this help message"

.PHONY: all clean debug run install-deps install-deps-mac help directories
