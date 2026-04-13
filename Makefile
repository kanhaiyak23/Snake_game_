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

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(ALL_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
