/*
 * main_wasm.c — Emscripten entry point for the browser build.
 *
 * Replaces main.c for the Wasm build. Key differences:
 *   - emscripten_set_main_loop() drives the game tick (no while+usleep).
 *   - emscripten_set_main_loop_timing() adjusts speed per level each tick.
 *   - No blocking start/game-over screens; auto-restarts after death.
 *
 * Allowed: <emscripten.h>, <time.h> for seeding RNG.
 */
#include <emscripten.h>
#include <time.h>

#include "../libs/math.h"
#include "../libs/memory.h"
#include "../libs/screen.h"
#include "../libs/keyboard.h"
#include "snake.h"

static GameState gs;
static int high_score  = 0;
static int dead_ticks  = 0;   /* ticks spent on game-over pause */

#define DEAD_PAUSE_TICKS 30   /* ~3 s at level-1 speed before auto-restart */

static void tick(void) {
    /* ---- Game-over pause: wait then auto-restart ---- */
    if (gs.game_over) {
        dead_ticks++;
        if (dead_ticks >= DEAD_PAUSE_TICKS) {
            if (gs.high_score > high_score) high_score = gs.high_score;
            game_free(&gs);
            game_init(&gs, high_score);
            dead_ticks = 0;
        }
        return;
    }

    /* ---- Normal game tick ---- */
    {
        char k = keyPressed();
        if (k) game_handle_input(&gs, k);
    }
    game_update(&gs);
    game_render(&gs);

    /* Adjust setTimeout interval to match current game speed. */
    emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT,
                                    game_speed_delay(&gs) / 1000);
}

int main(void) {
    mem_init();
    keyboard_init();
    screen_init();
    my_srand((unsigned int)time((void*)0));
    game_init(&gs, 0);
    game_render(&gs);

    /* 0 fps = timing controlled manually via set_main_loop_timing each tick. */
    emscripten_set_main_loop(tick, 0, 1);
    return 0;
}
