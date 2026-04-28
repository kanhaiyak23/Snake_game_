# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build & Run

```bash
make          # compile → ./snake
make run      # compile + launch immediately
make clean    # remove binary
./snake       # run directly after build
```

**Run unit tests** (no Makefile target — compile manually):
```bash
gcc -std=c99 -I./src/libs -I./src/game -o test_logic \
    src/libs/math.c src/libs/string.c src/libs/memory.c \
    src/game/snake.c src/libs/screen.c src/libs/keyboard.c \
    test_logic.c && ./test_logic
```

Requires GCC (C99), POSIX terminal (macOS/Linux), minimum terminal size ~65×28.

## Architecture

### The "no stdlib" constraint

This is an OS course project. **Do not use** `<string.h>`, `<math.h>`, `malloc`, `free`, or `rand`. Every one of those is replaced by a custom library in `src/libs/`. The only stdlib allowances are `printf`/`putchar` (I/O) and `exit()` (process control).

### Library pipeline

```
keyboard.c  → raw termios input, escape-sequence state machine for arrow keys
string.c    → strlen/strcpy/strcmp/itoa/strtok replacements
memory.c    → 1 MiB static pool, first-fit allocator with forward coalescing
math.c      → arithmetic (no <math.h>), XorShift32 RNG, bounds/clamp helpers
screen.c    → ANSI/VT100 renderer; owns field geometry (FIELD_X/Y/W/H)
```

Each library is independent except that game code layers them all.

### Game code

- **`snake.h`** — all shared structs (`Segment`, `Snake`, `Food`, `GameState`) and the public API (`game_init`, `game_update`, `game_handle_input`, `game_render`, `game_free`, `game_speed_delay`).
- **`snake.c`** — full game logic: movement, collision, food spawning, scoring, level progression, lives/respawn.
- **`main.c`** — entry point; owns the startup sequence, screen flows (start screen → countdown → game loop → game-over screen), and the `usleep` frame-rate loop.

### Memory model

Every snake `Segment` lives in the custom pool (`memory.c`). On each tick: `alloc()` new head; if no food eaten, `dealloc()` old tail. On game over, `game_free()` walks the linked list and frees all nodes, then `mem_init()` resets the pool for a new game. Live pool usage is shown in the HUD via `mem_used()`.

### Key constants (in `snake.h`)

| Constant | Default | Meaning |
|---|---|---|
| `MAX_FOODS` | 3 | Simultaneous active food items |
| `INITIAL_LIVES` | 3 | Lives per session |
| `GAME_END_*` | 0–4 | Reason codes stored in `GameState.game_end_reason` |

Frame delay: `180000 µs − (15000 µs × (level−1))`, clamped to `[50000, 180000]` µs (level 1 = 180 ms/frame, level 10 = 50 ms/frame).

### Field geometry

`screen.c` owns field origin and size. Game code must call `screen_field_x/y/w/h()` accessors — never read the `FIELD_*` macros directly — so that `screen_update_layout()` can adjust geometry without breaking callers.
