/*
 * main_wasm.c — Emscripten entry point for the browser build.
 *
 * States:
 *   STATE_START    — show splash, wait for SPACE/ENTER
 *   STATE_PLAYING  — normal game tick
 *   STATE_DEAD     — game over pause before auto-restart
 */
#include <emscripten.h>
#include <time.h>

#include "../libs/math.h"
#include "../libs/string.h"
#include "../libs/memory.h"
#include "../libs/screen.h"
#include "../libs/keyboard.h"
#include "../game/snake.h"

#define STATE_START   0
#define STATE_PLAYING 1
#define STATE_DEAD    2

#define DEAD_PAUSE_TICKS 25

static GameState gs;
static int high_score = 0;
static int state      = STATE_START;
static int dead_ticks = 0;

static void hs_load(void) {
    high_score = EM_ASM_INT({
        return parseInt(localStorage.getItem('snakeHS') || '0', 10);
    });
}

static void hs_save(void) {
    EM_ASM({ localStorage.setItem('snakeHS', $0); }, high_score);
}

/* ── Start screen ──────────────────────────────────────────────── */
static void draw_start(void) {
    int cx, cy;
    screen_update_layout();
    cx = screen_field_x() + my_div(screen_field_w(), 2) - 14;
    cy = screen_field_y() + my_div(screen_field_h(), 2) - 4;
    if (cx < 2) cx = 2;

    screen_clear();

    /* Border */
    screen_set_fg(32);
    screen_draw_border(screen_field_x() - 1, screen_field_y() - 1,
                       screen_field_w(), screen_field_h());
    screen_reset_color();

    /* ASCII logo */
    screen_goto(cx, cy);
    screen_set_fg(92); screen_set_bold();
    screen_putstr("  ____              _        ");
    screen_goto(cx, cy + 1);
    screen_putstr(" / ___| _ __   __ _| | _____ ");
    screen_goto(cx, cy + 2);
    screen_putstr(" \\___ \\| '_ \\ / _` | |/ / _ \\");
    screen_goto(cx, cy + 3);
    screen_putstr("  ___) | | | | (_| |   <  __/");
    screen_goto(cx, cy + 4);
    screen_putstr(" |____/|_| |_|\\__,_|_|\\_\\___|");
    screen_reset_color();

    screen_goto(cx + 2, cy + 6);
    screen_set_fg(97);
    screen_putstr("WebAssembly Edition  --  Custom C Libraries");
    screen_reset_color();

    /* Controls */
    screen_goto(cx + 2, cy + 8);
    screen_set_fg(33);
    screen_putstr("+---------------------------------------+");
    screen_goto(cx + 2, cy + 9);
    screen_putstr("|  W/S/A/D or Arrow Keys  . . .  Move  |");
    screen_goto(cx + 2, cy + 10);
    screen_putstr("|  P  . . . . . . . . . . .  Pause     |");
    screen_goto(cx + 2, cy + 11);
    screen_putstr("|  Q  . . . . . . . . . . .  Quit      |");
    screen_goto(cx + 2, cy + 12);
    screen_putstr("+---------------------------------------+");
    screen_reset_color();

    screen_goto(cx + 4, cy + 14);
    screen_set_fg(96);
    screen_putstr("Eat consecutive foods to stack up to x8 combo!");
    screen_reset_color();

    /* Call to action — blink effect driven by dead_ticks reuse */
    screen_goto(cx + 6, cy + 16);
    screen_set_fg(95); screen_set_bold();
    screen_putstr(">>> Press SPACE or ENTER to Start <<<");
    screen_reset_color();

    screen_flush();
}

/* ── Game-over screen ──────────────────────────────────────────── */
static void draw_game_over(void) {
    int cx = screen_field_x() + my_div(screen_field_w(), 2) - 14;
    int cy = screen_field_y() + my_div(screen_field_h(), 2) - 5;
    if (cx < 2) cx = 2;

    char sbuf[16], hbuf[16], lbuf[8];
    my_itoa(gs.score,      sbuf);
    my_itoa(gs.high_score, hbuf);
    my_itoa(gs.lives,      lbuf);

    screen_clear();

    screen_set_fg(32);
    screen_draw_border(screen_field_x() - 1, screen_field_y() - 1,
                       screen_field_w(), screen_field_h());
    screen_reset_color();

    screen_goto(cx + 4, cy);
    screen_set_fg(91); screen_set_bold();
    screen_putstr("+==================================+");
    screen_goto(cx + 4, cy + 1);
    screen_putstr("|   ***  G A M E   O V E R  ***   |");
    screen_goto(cx + 4, cy + 2);
    screen_putstr("+==================================+");
    screen_reset_color();

    screen_goto(cx + 4, cy + 4);
    screen_set_fg(97); screen_putstr("  Final Score  :  ");
    screen_set_fg(92); screen_set_bold(); screen_putstr(sbuf);
    screen_reset_color();

    screen_goto(cx + 4, cy + 5);
    screen_set_fg(97); screen_putstr("  High Score   :  ");
    screen_set_fg(93); screen_set_bold(); screen_putstr(hbuf);
    screen_reset_color();

    screen_goto(cx + 4, cy + 7);
    screen_set_fg(95);
    screen_putstr("  Restarting in a moment...");
    screen_reset_color();

    screen_flush();
}

/* ── Main tick ─────────────────────────────────────────────────── */
static void tick(void) {
    char k = keyPressed();

    if (state == STATE_START) {
        draw_start();
        if (k == ' ' || k == '\n' || k == '\r') {
            game_init(&gs, high_score);
            state = STATE_PLAYING;
        }
        return;
    }

    if (state == STATE_DEAD) {
        draw_game_over();
        dead_ticks++;
        if (dead_ticks >= DEAD_PAUSE_TICKS) {
            if (gs.high_score > high_score) high_score = gs.high_score;
            game_free(&gs);
            state     = STATE_START;
            dead_ticks = 0;
        }
        return;
    }

    /* STATE_PLAYING */
    if (k) game_handle_input(&gs, k);
    game_update(&gs);
    game_render(&gs);
    emscripten_set_main_loop_timing(EM_TIMING_SETTIMEOUT,
                                    game_speed_delay(&gs) / 1000);
    if (gs.game_over) {
        if (gs.high_score > high_score) {
            high_score = gs.high_score;
            hs_save();
        }
        state     = STATE_DEAD;
        dead_ticks = 0;
    }
}

int main(void) {
    mem_init();
    keyboard_init();
    screen_init();
    my_srand((unsigned int)time((void*)0));
    hs_load();
    state = STATE_START;
    emscripten_set_main_loop(tick, 10, 1);
    return 0;
}
