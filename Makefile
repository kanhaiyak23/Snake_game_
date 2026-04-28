# Makefile — Snake Game (Custom C Libraries)

CC      = gcc
CFLAGS  = -Wall -Wextra -pedantic -std=c99 -g -I./src/libs -I./src/game

# Source files
LIB_SRCS = src/libs/math.c \
           src/libs/string.c \
           src/libs/memory.c \
           src/libs/screen.c \
           src/libs/keyboard.c

GAME_SRCS = src/game/snake.c \
            src/game/main.c

ALL_SRCS  = $(LIB_SRCS) $(GAME_SRCS)

# Output binary
TARGET = snake

# ── WebAssembly build via Emscripten ───────────────────────────────────────
EMCC       = emcc
WASM_FLAGS = -std=c99 -I./src/libs -I./src/game -O2 \
             -s EXPORTED_FUNCTIONS='["_main","_wasm_push_key"]' \
             -s EXPORTED_RUNTIME_METHODS='["FS"]' \
             --pre-js pre.js

# Use keyboard_wasm.c instead of keyboard.c; drop main.c (replaced by main_wasm.c)
WASM_SRCS  = src/libs/math.c \
             src/libs/string.c \
             src/libs/memory.c \
             src/libs/screen.c \
             src/game/snake.c \
             src/wasm/keyboard_wasm.c \
             src/wasm/main_wasm.c

.PHONY: all clean run wasm wasm-clean

all: $(TARGET)

$(TARGET): $(ALL_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)

wasm: $(WASM_SRCS) pre.js
	@which emcc > /dev/null 2>&1 || \
		(echo "ERROR: emcc not found. Install Emscripten: https://emscripten.org/docs/getting_started/downloads.html" && exit 1)
	mkdir -p docs
	$(EMCC) $(WASM_FLAGS) -o docs/snake.js $(WASM_SRCS)
	cp src/wasm/wasm_index.html docs/index.html
	@echo "Wasm build complete → docs/  (serve with: python3 -m http.server 8080 --directory docs)"

wasm-clean:
	rm -rf docs/snake.js docs/snake.wasm docs/index.html
