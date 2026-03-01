CC = gcc
CFLAGS = -Wall -Wextra -Wno-unused-parameter -O2 -I./include -D_POSIX_C_SOURCE=200809L
LDFLAGS = 
LIBS = 

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LIBS += -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lcrypto -lssl
endif
ifeq ($(UNAME_S),Darwin)
    LIBS += -lraylib -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm -lpthread -lcrypto -lssl
    CFLAGS += -I/opt/homebrew/opt/raylib/include -I/opt/homebrew/opt/openssl/include
    LDFLAGS += -L/opt/homebrew/opt/raylib/lib -L/opt/homebrew/opt/openssl/lib
endif

# Add AddressSanitizer support
asan: CFLAGS += -fsanitize=address -g -O1 -fno-omit-frame-pointer
asan: LDFLAGS += -fsanitize=address
asan: all

debug: CFLAGS += -g -DDEBUG -O0
debug: all

OBJDIR = obj
BINDIR = bin
SRCDIR = src

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
DEPS = $(OBJECTS:.o=.d)
TARGET = $(BINDIR)/stegachat

all: directories $(TARGET)

directories:
	@mkdir -p $(OBJDIR) $(BINDIR)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

-include $(DEPS)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	rm -f *.png *.wav temp_encoded_* encoded_* received_* steganet.log*
	clear

install-deps:
	sudo apt update
	sudo apt install -y libraylib-dev libssl-dev

install-deps-mac:
	brew install raylib openssl

install: all
	install -d /usr/local/bin
	install -m 755 $(TARGET) /usr/local/bin/stegachat

run: $(TARGET)
	cd $(BINDIR) && ./stegachat

.PHONY: all clean asan debug run install-deps install-deps-mac install directories
