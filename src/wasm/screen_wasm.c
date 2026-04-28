/*
 * screen_wasm.c — Wasm screen implementation.
 *
 * Buffers all output (ANSI escape sequences + text) in a local byte
 * array.  screen_flush() ships the entire frame to xterm.js in one
 * EM_ASM call using HEAPU8.slice() → Uint8Array → term.write().
 *
 * Using Uint8Array lets xterm.js decode multi-byte UTF-8 sequences
 * (box-drawing chars ╔═╗║╚╝) correctly — something char-by-char
 * or string approaches cannot guarantee.
 */
#include "screen.h"
#include <emscripten.h>

/* ── Output buffer (one full frame) ───────────────────────────── */
static unsigned char obuf[65536];
static int           opos = 0;

static void ob(unsigned char c)   { if (opos < 65534) obuf[opos++] = c; }
static void os(const char *s)     { while (*s) ob((unsigned char)*s++); }
static void oi(int n) {
    if (n <= 0) { ob('0'); return; }
    char tmp[12]; int len = 0;
    while (n > 0) { tmp[len++] = (char)('0' + n % 10); n /= 10; }
    int i; for (i = len - 1; i >= 0; i--) ob((unsigned char)tmp[i]);
}

/* ── Fixed field geometry (80×34 xterm terminal) ──────────────── */
/* fx=5: hud_col=4, separator fits cols 4-77, symmetric-enough margins */
static int fx = 5, fy = 4, fw = 72, fh = 24;

/* ── Public API ────────────────────────────────────────────────── */

void screen_update_layout(void) { /* fixed in browser — no-op */ }

void screen_init(void) {
    os("\033[?1049h");    /* enter alternate screen buffer */
    os("\033[2J\033[H");  /* clear                         */
    os("\033[?25l");      /* hide cursor                   */
    screen_flush();
}

void screen_cleanup(void) {
    os("\033[?25h\033[?1049l");
    screen_flush();
}

void screen_clear(void)         { os("\033[2J\033[H"); }
void screen_hide_cursor(void)   { os("\033[?25l");     }
void screen_show_cursor(void)   { os("\033[?25h");     }
void screen_set_bold(void)      { os("\033[1m");       }
void screen_reset_color(void)   { os("\033[0m");       }
void screen_putchar(char c)     { ob((unsigned char)c); }
void screen_putstr(const char *s) { os(s);             }

void screen_goto(int col, int row) {
    os("\033["); oi(row); ob(';'); oi(col); ob('H');
}

void screen_set_fg(int code) { os("\033["); oi(code); ob('m'); }
void screen_set_bg(int code) { os("\033["); oi(code); ob('m'); }

void screen_draw_border(int x, int y, int w, int h) {
    /* No-op for the Wasm build.
     * The HTML outer container already provides an animated glowing border.
     * Attempting to draw ASCII/Unicode borders via ANSI causes unavoidable
     * visual misalignment due to font-metric drift in xterm.js. */
    (void)x; (void)y; (void)w; (void)h;
}

/* Ship the frame buffer to xterm.js as raw UTF-8 bytes. */
void screen_flush(void) {
    if (opos == 0) return;
    EM_ASM({
        if (window.__termWrite) {
            window.__termWrite(HEAPU8.slice($0, $0 + $1));
        }
    }, obuf, opos);
    opos = 0;
}

int screen_field_x(void) { return fx; }
int screen_field_y(void) { return fy; }
int screen_field_w(void) { return fw; }
int screen_field_h(void) { return fh; }
