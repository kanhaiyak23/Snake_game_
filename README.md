# Snake — Terminal Game in C (OS Final Project)

A real-time Snake game that runs entirely in the terminal, written in C99 from scratch.
No standard library functions for logic (`strlen`, `malloc`, `free`, `rand`, etc.) are used — every
subsystem is implemented as one of five custom libraries.

---

## Features

| Feature | Details |
|---------|---------|
| Smooth gameplay | Non-blocking keyboard input, configurable frame rate |
| 10 difficulty levels | Speed increases every 50 points; level 10 is maximum |
| Level progress bar | Live bar shows exact progress toward next level |
| Persistent high score | Best score carried across all games in a session |
| Foods-eaten counter | Running total of food items consumed |
| Start screen | ASCII-art logo, controls reference, tips |
| Countdown | 3-2-1-GO! before each game so the player is ready |
| Pause/resume | Press `P` at any time during play |
| Game-over screen | Final stats (score, high score, length, food), **Play Again** or **Quit** |
| New-high-score alert | Celebrated on the game-over screen if you beat your best |
| Memory debug readout | Live byte count from the custom pool allocator |
| Zero leaks | Every `Segment` is `dealloc()`'d on game over |

---

## Controls

| Key | Action |
|-----|--------|
| `W` / `↑` | Move Up |
| `S` / `↓` | Move Down |
| `A` / `←` | Move Left |
| `D` / `→` | Move Right |
| `P` | Pause / Resume |
| `Q` | Quit game |
| `SPACE` / `ENTER` | Confirm on menu screens |

---

## Build & Run

```bash
# Compile
make

# Play
./snake

# Compile and play in one step
make run

# Remove the binary
make clean
```

**Requirements:** GCC (or any C99-compatible compiler), POSIX terminal (macOS / Linux).  
**Minimum terminal size:** ~65 × 28 characters.

---

## Project Structure

```
os_project_2/
├── src/
│   ├── libs/                     # Five custom C libraries
│   │   ├── math.c / math.h       # Arithmetic engine (no <math.h>)
│   │   ├── string.c / string.h   # String utilities (no <string.h>)
│   │   ├── memory.c / memory.h   # 1 MiB virtual pool allocator
│   │   ├── screen.c / screen.h   # ANSI terminal renderer
│   │   └── keyboard.c / keyboard.h  # Non-blocking keyboard input
│   └── game/
│       ├── snake.h               # Data structures + API declarations
│       ├── snake.c               # Full game logic
│       └── main.c                # Entry point, screen flows, game loop
├── test_logic.c                  # Standalone unit-test harness
├── Makefile
└── README.md
```

---

## Architecture

### Library Pipeline

Each library feeds into the next like a pipeline:

```
keyboard.c  →  captures WASD / arrow key input (raw termios mode)
    ↓
string.c    →  my_itoa() converts score integers to display strings
    ↓
memory.c    →  alloc() / dealloc() manages every snake Segment struct
    ↓
math.c      →  my_in_bounds(), my_div(), my_mod() handle movement & levels
    ↓
screen.c    →  renders border, snake, food, HUD, and menus each frame
```

### Game State Machine

```
  ┌─────────────┐      SPACE/ENTER       ┌──────────────┐
  │ Start Screen│ ─────────────────────► │  Countdown   │
  └─────────────┘                        └──────┬───────┘
                                                │  GO!
                                         ┌──────▼───────┐
                            ┌── P ──────►│  Game Loop   │◄─── play again
                            └── P ───────│  (running)   │
                                         └──────┬───────┘
                                                │  wall / self / Q
                                         ┌──────▼───────┐
                                         │  Game Over   │
                                         │   Screen     │
                                         └──┬───────┬───┘
                                SPACE/ENTER │       │ Q
                                            ▼       ▼
                                       play again  exit
```

### Memory Model

The virtual RAM pool (`memory.c`) is a 1 MiB static array managed with a first-fit free-list.
Every snake `Segment` is a heap-like object inside this pool:

| Game event | Memory action |
|------------|---------------|
| Snake moves (no food) | `alloc()` new head + `dealloc()` old tail |
| Snake eats food | `alloc()` new head; tail is **not** freed (snake grows) |
| Game over | `game_free()` walks the full linked list and `dealloc()`s every node |
| New game starts | `mem_init()` resets the pool for a clean slate |

Live memory usage is displayed in the HUD so you can watch allocations in real time.

---

## Library Reference

### `math.c` / `math.h`

| Function | Purpose |
|----------|---------|
| `my_mul(a, b)` | Integer multiplication via repeated addition |
| `my_div(a, b)` | Integer division via repeated subtraction |
| `my_mod(a, b)` | Modulo via repeated subtraction |
| `my_abs(x)` | Absolute value |
| `my_clamp(v, lo, hi)` | Clamp value to range |
| `my_in_bounds(x, y, w, h)` | Wall-collision test |
| `my_max / my_min` | Min/max helpers |
| `my_srand / my_rand` | XorShift32 PRNG (O(1), full-period 2³²−1) |
| `my_rand_range(lo, hi)` | Bounded random integer |

> XorShift32 was chosen over LCG because `my_mul` uses repeated addition — seeding with a large
> `time()` value would require billions of iterations. XorShift32 is three XOR-shift ops regardless
> of seed magnitude.

### `string.c` / `string.h`

Custom implementations of `strlen`, `strcpy`, `strcmp`, `strncmp`, `strcat`, `itoa`, `strtok`.
Used primarily for converting integer scores to printable strings.

### `memory.c` / `memory.h`

First-fit allocator over a `POOL_SIZE` (1 MiB) static buffer with:
- 8-byte alignment
- Block splitting on `alloc()`
- Forward coalescing of free blocks on `dealloc()`
- `mem_used()` — sum of all currently allocated bytes

### `screen.c` / `screen.h`

Pure-ANSI renderer (`printf` / `putchar` only):

| Function | Purpose |
|----------|---------|
| `screen_init()` | Enter alternate screen buffer, hide cursor |
| `screen_cleanup()` | Restore original buffer, show cursor |
| `screen_clear()` | `ESC[2J` — full clear |
| `screen_goto(col, row)` | `ESC[row;colH` — absolute cursor move |
| `screen_draw_border(x,y,w,h)` | ASCII `+`/`-`/`|` box |
| `screen_set_fg(code)` | ANSI foreground colour (30–37, 90–97) |
| `screen_set_bg(code)` | ANSI background colour (40–47, 100–107) |
| `screen_set_bold()` | Bold text (`ESC[1m`) |
| `screen_reset_color()` | `ESC[0m` — reset all SGR attributes |
| `screen_flush()` | `fflush(stdout)` |

Field layout constants (`FIELD_X`, `FIELD_Y`, `FIELD_W`, `FIELD_H`) live here so every
module uses the same coordinate system.

### `keyboard.c` / `keyboard.h`

Uses POSIX `termios` to switch to raw, non-blocking mode:
- `keyboard_init()` — save terminal state, enter raw mode, set `O_NONBLOCK`
- `keyboard_restore()` — restore original `termios` settings
- `keyPressed()` — `read(STDIN_FILENO, ...)` — returns char or `0` if no key
- Arrow keys are decoded from their three-byte escape sequences to `w/s/a/d`

---

## Scoring & Progression

| Event | Score gain |
|-------|-----------|
| Eat one food item | `10 × current_level` points |
| Level threshold | Every 50 cumulative points → level up |
| Max level | Level 10 (speed floors at 50 ms/frame) |

Frame delay formula (via `math.c`, no hard-coded lookup):

```
delay = 180 000 µs  −  (15 000 µs × (level − 1))
delay = clamp(delay, 50 000, 180 000)
```

---

## Rules Compliance

| Rule | Implementation |
|------|----------------|
| No `<string.h>` | All string ops in `src/libs/string.c` |
| No `<math.h>` | All arithmetic in `src/libs/math.c` |
| No `malloc` / `free` | Custom pool allocator in `src/libs/memory.c` |
| No hard-coded logic values | Positions, speeds, and scores all computed dynamically |
| `<stdio.h>` only for I/O | `printf` / `putchar` for terminal rendering |
| `<stdlib.h>` only for `exit()` | Process control only |
| Dynamic alloc / dealloc | Each `Segment` is `alloc()`'d; tail is `dealloc()`'d every move frame |
| Library pipeline | keyboard → string → memory → math → screen |

---

## Running the Unit Tests

```bash
gcc -std=c99 -I./src/libs -I./src/game -o test_logic \
    src/libs/math.c src/libs/string.c src/libs/memory.c \
    src/game/snake.c src/libs/screen.c src/libs/keyboard.c \
    test_logic.c && ./test_logic
```

Tests cover: `math.c` arithmetic & RNG, `string.c` itoa & strcmp,
`memory.c` alloc/dealloc/coalesce, and `game_init` / direction-reversal protection.

---

## Known Limitations

- Requires a terminal that supports ANSI/VT100 escape sequences (all modern macOS/Linux terminals do).
- Minimum recommended terminal size: **65 × 28** characters.
- `my_mul` uses repeated addition — this is intentional (no `<math.h>`) and is fine at game scale
  (max multiplier is 10 for level × score-per-food).
