/*
 * screen.c — Custom Terminal Screen Library Implementation
 * Uses pure ANSI escape sequences with printf/putchar from <stdio.h>.
 * No additional libraries required.
 */
#include "screen.h"
#include "string.h"
#include <stdio.h>   /* allowed: terminal I/O only */
#include <sys/ioctl.h>
#include <unistd.h>

static int layout_field_x = FIELD_X;
static int layout_field_y = FIELD_Y;
static int layout_field_w = FIELD_W;
static int layout_field_h = FIELD_H;

static int clamp_int(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void screen_update_layout(void) {
    struct winsize ws;
    int term_w = 80;
    int term_h = 24;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0) {
        if (ws.ws_col > 0) term_w = (int)ws.ws_col;
        if (ws.ws_row > 0) term_h = (int)ws.ws_row;
    }

    {
        int min_w = 20;
        int min_h = 10;
        int max_w = term_w - 2;
        int max_h = term_h - 6;
        int border_w;
        int border_x;

        if (max_w < min_w) max_w = min_w;
        if (max_h < min_h) max_h = min_h;

        layout_field_w = clamp_int(FIELD_W, min_w, max_w);
        layout_field_h = clamp_int(FIELD_H, min_h, max_h);

        border_w = layout_field_w + 2;
        border_x = (term_w - border_w) / 2 + 1;
        if (border_x < 1) border_x = 1;

        layout_field_x = border_x + 1;
        layout_field_y = 3;
    }
}

/* ---------------------------------------------------------------
 * screen_init: switch to alternate screen buffer and clear it.
 * The alternate buffer is isolated — no scroll history contamination.
 * This is the standard approach for terminal games (vim, htop, etc.).
 * --------------------------------------------------------------- */
void screen_init(void) {
    screen_update_layout();
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
    int i, row;

    /* Top edge: ╔═══╗ */
    screen_goto(x, y);
    screen_putstr("\xe2\x95\x94");              /* ╔ */
    for (i = 0; i < w; i++) screen_putstr("\xe2\x95\x90"); /* ═ */
    screen_putstr("\xe2\x95\x97");              /* ╗ */

    /* Side edges: ║ */
    for (row = 1; row <= h; row++) {
        screen_goto(x, y + row);
        screen_putstr("\xe2\x95\x91");          /* ║ */
        screen_goto(x + w + 1, y + row);
        screen_putstr("\xe2\x95\x91");          /* ║ */
    }

    /* Bottom edge: ╚═══╝ */
    screen_goto(x, y + h + 1);
    screen_putstr("\xe2\x95\x9a");              /* ╚ */
    for (i = 0; i < w; i++) screen_putstr("\xe2\x95\x90"); /* ═ */
    screen_putstr("\xe2\x95\x9d");              /* ╝ */
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

int screen_field_x(void) { return layout_field_x; }
int screen_field_y(void) { return layout_field_y; }
int screen_field_w(void) { return layout_field_w; }
int screen_field_h(void) { return layout_field_h; }
