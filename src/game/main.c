/*
 * main.c — Snake Game Entry Point
 *
 * Startup sequence:
 *   1. mem_init()         — virtual RAM pool (memory.c)
 *   2. keyboard_init()    — raw non-blocking terminal (keyboard.c)
 *   3. screen_init()      — alternate screen buffer, hide cursor (screen.c)
 *   4. my_srand(seed)     — seed XorShift32 RNG from wall-clock (math.c)
 *   5. show_start_screen  — animated splash, wait for SPACE/ENTER
 *   6. game_init()        — build snake + place food (snake.c)
 *   7. game loop          — poll → handle input → update → render → sleep
 *   8. show_game_over_screen — play again / quit choice
 *   9. game_free()        — explicit dealloc of all segments (memory.c)
 *  10. screen_cleanup()   — restore original terminal buffer
 *  11. keyboard_restore() — restore terminal mode
 *
 * Allowed from <stdlib.h>: exit() only.
 * Allowed from <stdio.h>:  printf/putchar for terminal I/O.
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "../libs/math.h"
#include "../libs/string.h"
#include "../libs/memory.h"
#include "../libs/screen.h"
#include "../libs/keyboard.h"
#include "../libs/sound.h"
#include "snake.h"

/* ---------------------------------------------------------------
 * drain_input: flush any buffered keystrokes.
 * Called after menus so stale input doesn't bleed into the game.
 * 50 × 2 ms = 100 ms window — enough to drain multi-byte escape
 * sequences that arrow keys produce.
 * --------------------------------------------------------------- */
static void drain_input(void) {
    int i;
    for (i = 0; i < 50; i++) {
        (void)keyPressed();
        usleep(2000);
    }
}

/* ---------------------------------------------------------------
 * draw_hline: draw a horizontal rule of `len` dashes at (col,row).
 * Avoids repeating the loop in every screen-drawing function.
 * --------------------------------------------------------------- */
static void draw_hline(int col, int row, int len) {
    int i;
    screen_goto(col, row);
    screen_set_fg(90);
    for (i = 0; i < len; i++) screen_putchar('-');
    screen_reset_color();
}

/* ---------------------------------------------------------------
 * show_start_screen: full-featured splash screen.
 *   - ASCII-art "Snake" logo
 *   - Bordered controls reference
 *   - Tips
 *   - Waits for SPACE or ENTER
 * --------------------------------------------------------------- */
static void show_start_screen(void) {
    screen_clear();

    /* ---- ASCII art logo ---- */
    screen_goto(10, 2);
    screen_set_fg(92);
    screen_set_bold();
    screen_putstr("  ____              _        ");
    screen_goto(10, 3);
    screen_putstr(" / ___| _ __   __ _| | _____ ");
    screen_goto(10, 4);
    screen_putstr(" \\___ \\| '_ \\ / _` | |/ / _ \\");
    screen_goto(10, 5);
    screen_putstr("  ___) | | | | (_| |   <  __/");
    screen_goto(10, 6);
    screen_putstr(" |____/|_| |_|\\__,_|_|\\_\\___|");
    screen_reset_color();

    /* Subtitle */
    screen_goto(10, 8);
    screen_set_fg(97);
    screen_putstr("  Terminal Snake  --  Custom C Libraries Edition");
    screen_reset_color();

    draw_hline(9, 9, 50);

    /* ---- Controls box ---- */
    screen_goto(10, 10);
    screen_set_fg(33);
    screen_set_bold();
    screen_putstr("  CONTROLS");
    screen_reset_color();

    screen_goto(9, 11);
    screen_set_fg(33);
    screen_putstr("  +---------------------------------------------+");
    screen_goto(9, 12);
    screen_putstr("  |  W / Up Arrow     . . . . .   Move Up       |");
    screen_goto(9, 13);
    screen_putstr("  |  S / Down Arrow   . . . . .   Move Down     |");
    screen_goto(9, 14);
    screen_putstr("  |  A / Left Arrow   . . . . .   Move Left     |");
    screen_goto(9, 15);
    screen_putstr("  |  D / Right Arrow  . . . . .   Move Right    |");
    screen_goto(9, 16);
    screen_putstr("  |  P                . . . . .   Pause/Resume  |");
    screen_goto(9, 17);
    screen_putstr("  |  Q                . . . . .   Quit Game     |");
    screen_goto(9, 18);
    screen_putstr("  +---------------------------------------------+");
    screen_reset_color();

    draw_hline(9, 19, 50);

    /* ---- Tips ---- */
    screen_goto(10, 20);
    screen_set_fg(96);
    screen_putstr("  Eat ");
    screen_set_fg(91);
    screen_putstr("@");
    screen_set_fg(96);
    screen_putstr(" to grow.  Avoid walls and your own body!");
    screen_goto(10, 21);
    screen_putstr("  Higher score = faster play (speed scales with your score).");
    screen_reset_color();

    draw_hline(9, 22, 50);

    /* ---- Call to action ---- */
    screen_goto(10, 23);
    screen_set_fg(95);
    screen_set_bold();
    screen_putstr("       >>> Press SPACE or ENTER to Start <<<");
    screen_reset_color();

    screen_flush();

    drain_input();
    {
        char k = 0;
        while (k != ' ' && k != '\n' && k != '\r') {
            k = keyPressed();
            usleep(15000);
        }
    }
    drain_input();
}

/* ---------------------------------------------------------------
 * show_mode_menu: let the player choose Classic or Wrap mode.
 * Returns 0 for Classic, 1 for Wrap.
 * --------------------------------------------------------------- */
static int show_mode_menu(void) {
    int sel = 0;
    for (;;) {
        screen_clear();

        screen_goto(10, 3);
        screen_set_fg(92); screen_set_bold();
        screen_putstr("  SELECT GAME MODE");
        screen_reset_color();

        draw_hline(9, 4, 50);

        /* Option 0 */
        screen_goto(10, 6);
        if (sel == 0) {
            screen_set_fg(92); screen_set_bold();
            screen_putstr("  > [ CLASSIC MODE ]");
        } else {
            screen_set_fg(90);
            screen_putstr("    [ CLASSIC MODE ]");
        }
        screen_reset_color();
        screen_goto(14, 7);
        screen_set_fg(90);
        screen_putstr("Hit a wall or obstacle = lose a life");
        screen_reset_color();

        /* Option 1 */
        screen_goto(10, 9);
        if (sel == 1) {
            screen_set_fg(96); screen_set_bold();
            screen_putstr("  > [ WRAP MODE    ]");
        } else {
            screen_set_fg(90);
            screen_putstr("    [ WRAP MODE    ]");
        }
        screen_reset_color();
        screen_goto(14, 10);
        screen_set_fg(90);
        screen_putstr("Exit right wall -> appear from left  (obstacles still lethal)");
        screen_reset_color();

        draw_hline(9, 12, 50);
        screen_goto(10, 13);
        screen_set_fg(33);
        screen_putstr("  W/S or Up/Down to navigate    SPACE/ENTER to confirm");
        screen_reset_color();

        screen_flush();

        drain_input();
        {
            char k = 0;
            while (!k) { k = keyPressed(); usleep(15000); }
            if (k == KEY_UP   || k == 'w' || k == 'W') sel = 0;
            if (k == KEY_DOWN || k == 's' || k == 'S') sel = 1;
            if (k == ' ' || k == '\n' || k == '\r')    return sel;
        }
    }
}

/* ---------------------------------------------------------------
 * show_countdown: animated 3-2-1-GO! before each game.
 * Gives the player time to position their fingers.
 * --------------------------------------------------------------- */
static void show_countdown(void) {
    int centre_col;
    int centre_row;
    int i;
    char dbuf[4];
    screen_update_layout();
    centre_col = screen_field_x() + my_div(screen_field_w(), 2) - 1;
    centre_row = screen_field_y() + my_div(screen_field_h(), 2);

    for (i = 3; i >= 1; i--) {
        screen_goto(centre_col, centre_row);
        screen_set_fg(93);
        screen_set_bold();
        my_itoa(i, dbuf);
        screen_putstr(dbuf);
        screen_reset_color();
        screen_flush();
        usleep(700000);
    }

    screen_goto(centre_col - 1, centre_row);
    screen_set_fg(92);
    screen_set_bold();
    screen_putstr("GO!");
    screen_reset_color();
    screen_flush();
    usleep(400000);
}

/* ---------------------------------------------------------------
 * show_game_over_screen: shown after every game session ends.
 *
 * Parameters:
 *   score      — final score this game
 *   high_score — overall best score (may equal score if new record)
 *   length     — snake length at time of death
 *   foods      — total food items eaten this game
 *   new_high   — 1 if score beat the previous session high score
 *   end_reason — GAME_END_* from snake.h (OOM / quit / collision)
 *
 * Returns 1 to play again, 0 to quit.
 * --------------------------------------------------------------- */
static int show_game_over_screen(int score, int high_score,
                                  int length, int foods, int lives_left, int new_high,
                                  int end_reason) {
    screen_clear();

    /* ---- GAME OVER banner ---- */
    screen_goto(8, 2);
    screen_set_fg(31);
    screen_putstr("+==================================================+");
    screen_goto(8, 3);
    screen_putstr("|                                                  |");
    screen_goto(8, 4);
    screen_set_fg(91);
    screen_set_bold();
    screen_putstr("|        ***   G A M E   O V E R   ***             |");
    screen_reset_color();
    screen_set_fg(31);
    screen_goto(8, 5);
    screen_putstr("|                                                  |");
    screen_goto(8, 6);
    screen_putstr("+==================================================+");
    screen_reset_color();

    /* ---- Reason: memory / quit (collision needs no extra line) ---- */
    if (end_reason == GAME_END_OOM) {
        screen_goto(8, 7);
        screen_set_fg(93);
        screen_set_bold();
        screen_putstr(
            "  ! Memory: alloc() failed — 1 MB heap full or fragmented.");
        screen_reset_color();
    } else if (end_reason == GAME_END_QUIT) {
        screen_goto(10, 7);
        screen_set_fg(96);
        screen_putstr("  (You quit with Q.)");
        screen_reset_color();
    } else if (end_reason == GAME_END_INTERNAL) {
        screen_goto(8, 7);
        screen_set_fg(91);
        screen_set_bold();
        screen_putstr("  ! Internal error: snake list invariant failed.");
        screen_reset_color();
    }

    /* ---- Stats ----
     * Snapshot values are passed in from main loop before game_free(),
     * so this screen can safely display final run data.
     */
    char sbuf[16], hbuf[16], lenbuf[16], fbuf[16], lbuf[16];
    my_itoa(score,  sbuf);
    my_itoa(high_score, hbuf);
    my_itoa(length, lenbuf);
    my_itoa(foods,  fbuf);
    my_itoa(lives_left, lbuf);

    screen_goto(10, 8);
    screen_set_fg(97);
    screen_putstr("  Final Score   :  ");
    screen_set_fg(92);
    screen_set_bold();
    screen_putstr(sbuf);
    screen_reset_color();

    screen_goto(10, 9);
    screen_set_fg(97);
    screen_putstr("  High Score    :  ");
    screen_set_fg(93);
    screen_set_bold();
    screen_putstr(hbuf);
    screen_reset_color();

    screen_goto(10, 10);
    screen_set_fg(97);
    screen_putstr("  Snake Length  :  ");
    screen_set_fg(95);
    screen_putstr(lenbuf);
    screen_reset_color();

    screen_goto(10, 11);
    screen_set_fg(97);
    screen_putstr("  Foods Eaten   :  ");
    screen_set_fg(91);
    screen_putstr(fbuf);
    screen_reset_color();

    screen_goto(10, 12);
    screen_set_fg(97);
    screen_putstr("  Lives Left    :  ");
    screen_set_fg(96);
    screen_putstr(lbuf);
    screen_reset_color();

    /* New high-score celebration */
    if (new_high) {
        screen_goto(10, 14);
        screen_set_fg(93);
        screen_set_bold();
        screen_putstr("  *** New Personal Best! Congratulations! ***");
        screen_reset_color();
    }

    draw_hline(9, 16, 50);

    /* ---- Options — two distinct visual buttons ---- */
    screen_goto(9, 17);
    screen_set_fg(92);
    screen_putstr("  +----------------------+  +----------------------+");
    screen_goto(9, 18);
    screen_putstr("  |   SPACE  /  ENTER    |  |          Q           |");
    screen_goto(9, 19);
    screen_set_bold();
    screen_putstr("  |     > Play Again     |  |     > Quit Game      |");
    screen_reset_color();
    screen_set_fg(92);
    screen_goto(9, 20);
    screen_putstr("  +----------------------+  +----------------------+");
    screen_reset_color();

    screen_flush();

    drain_input();
    for (;;) {
        char k = keyPressed();
        if (k == ' ' || k == '\n' || k == '\r') return 1;
        if (k == 'q' || k == 'Q')               return 0;
        usleep(15000);
    }
}

/* ---------------------------------------------------------------
 * main: top-level entry point and outer game loop.
 * --------------------------------------------------------------- */
int main(void) {
    /* 1. Virtual RAM pool */
    mem_init();

    /* 2. Raw non-blocking terminal input */
    keyboard_init();
    drain_input();

    /* 3. Alternate screen buffer, hide cursor */
    screen_init();

    /* 4. Seed XorShift32 RNG from wall-clock time */
    my_srand((unsigned int)time((void*)0));

    show_start_screen();

    /* ---- Outer loop: play-again logic ---- */
    int running    = 1;
    int high_score = 0;   /* persists across sessions within one run */

    while (running) {
        int prev_high = high_score;

        /* Let the player pick Classic or Wrap mode */
        int mode = show_mode_menu();
        drain_input();

        /* 5. Initialise game state — carry high score forward */
        GameState gs;
        game_init(&gs, high_score, mode);
        sound_game_start();

        /* Show initial frame then the 3-2-1 countdown */
        game_render(&gs);
        screen_flush();
        show_countdown();
        drain_input();

        /* ---- Inner loop: one game session ---- */
        while (!gs.game_over) {
            char k = keyPressed();
            if (k) game_handle_input(&gs, k);

            game_update(&gs);
            game_render(&gs);

            usleep((unsigned int)game_speed_delay(&gs));
        }

        /* Capture stats before freeing memory (game_free clears snake nodes). */
        int final_score  = gs.score;
        int final_high   = gs.high_score;
        int final_length = gs.snake.length;
        int final_foods  = gs.foods_eaten;
        int final_lives  = gs.lives;
        int is_new_high  = (final_score > prev_high && final_score > 0);

        if (final_high > high_score) high_score = final_high;

        /* 6. Release all segment memory */
        game_free(&gs);

        running = show_game_over_screen(
            final_score, final_high, final_length, final_foods, final_lives,
            is_new_high,
            gs.game_end_reason
        );
    }

    /* 7. Restore terminal */
    screen_cleanup();
    keyboard_restore();

    printf("Thanks for playing Snake! Goodbye.\n");
    return 0;
}
