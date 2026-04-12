/*
 * screen.c — Custom Terminal Screen Library Implementation
 * Uses pure ANSI escape sequences with printf/putchar from <stdio.h>.
 * No additional libraries required.
 */
#include "screen.h"
#include "string.h"
#include <stdio.h>   /* allowed: terminal I/O only */

/* ---------------------------------------------------------------
 * screen_init: switch to alternate screen buffer and clear it.
 * The alternate buffer is isolated — no scroll history contamination.
 * This is the standard approach for terminal games (vim, htop, etc.).
 * --------------------------------------------------------------- */
void screen_init(void) {
    printf("\033[?1049h");  /* enter alternate screen buffer */
    printf("\033[2J\033[H"); /* clear it                      */
    screen_hide_cursor();
    fflush(stdout);
}

/* ---------------------------------------------------------------
 * screen_cleanup: restore the original screen buffer.
 * Called on exit to leave the terminal clean.
 * --------------------------------------------------------------- */
void screen_cleanup(void) {
    screen_show_cursor();
    printf("\033[?1049l");  /* leave alternate screen buffer */
    fflush(stdout);
}

/* ---------------------------------------------------------------
 * screen_clear: clears the terminal and moves cursor to (1,1).
 * Uses ANSI escape: ESC[2J = clear screen, ESC[H = home cursor.
 * --------------------------------------------------------------- */
void screen_clear(void) {
    printf("\033[2J\033[H");
}

/* ---------------------------------------------------------------
 * screen_goto: move cursor to column `col`, row `row` (1-indexed).
 * ANSI: ESC[row;colH
 * --------------------------------------------------------------- */
void screen_goto(int col, int row) {
    printf("\033[%d;%dH", row, col);
}

/* ---------------------------------------------------------------
 * screen_putchar / screen_putstr: render a character / string
 * at the current cursor position.
 * --------------------------------------------------------------- */
void screen_putchar(char c) {
    putchar(c);
}

void screen_putstr(const char *s) {
    int i = 0;
    while (s[i] != '\0') {
        putchar(s[i]);
        i++;
    }
}

/* ---------------------------------------------------------------
 * Cursor visibility toggles.
 * --------------------------------------------------------------- */
void screen_hide_cursor(void) {
    printf("\033[?25l");
}

void screen_show_cursor(void) {
    printf("\033[?25h");
}

/* ---------------------------------------------------------------
 * screen_draw_border: draws a box at position (x,y) with inner
 * dimensions w×h using box-drawing characters.
 * x,y are the top-left corner column/row (1-indexed).
 * The box itself is drawn around the inner area, so total size
 * is (w+2) × (h+2).
 * --------------------------------------------------------------- */
void screen_draw_border(int x, int y, int w, int h) {
    int i;

    /* Top edge */
    screen_goto(x, y);
    screen_putchar('+');
    for (i = 0; i < w; i++) screen_putchar('-');
    screen_putchar('+');

    /* Side edges */
    int row;
    for (row = 1; row <= h; row++) {
        screen_goto(x, y + row);
        screen_putchar('|');
        screen_goto(x + w + 1, y + row);
        screen_putchar('|');
    }

    /* Bottom edge */
    screen_goto(x, y + h + 1);
    screen_putchar('+');
    for (i = 0; i < w; i++) screen_putchar('-');
    screen_putchar('+');
}

/* ---------------------------------------------------------------
 * Color control using ANSI SGR codes.
 * screen_set_fg(32) → green, (31) → red, (33) → yellow, etc.
 * --------------------------------------------------------------- */
void screen_set_fg(int code) {
    printf("\033[%dm", code);
}

void screen_set_bg(int code) {
    printf("\033[%dm", code);
}

void screen_set_bold(void) {
    printf("\033[1m");
}

void screen_reset_color(void) {
    printf("\033[0m");
}

/* ---------------------------------------------------------------
 * screen_flush: flush stdout so rendering appears immediately.
 * --------------------------------------------------------------- */
void screen_flush(void) {
    fflush(stdout);
}
