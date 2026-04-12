/*
 * keyboard.c — Custom Keyboard Input Library
 *
 * Uses POSIX termios to put the terminal into raw/non-blocking mode.
 * This is classified as the "Hardware Abstraction Exception" — we
 * are simulating keyboard hardware access.
 *
 * Only <stdio.h> is used for I/O, plus POSIX headers for terminal config.
 */
#include "keyboard.h"
#include <stdio.h>           /* allowed: terminal I/O                    */
#include <termios.h>         /* POSIX — hardware abstraction for keyboard */
#include <unistd.h>          /* read(), STDIN_FILENO                      */
#include <fcntl.h>           /* fcntl, O_NONBLOCK                         */

static struct termios old_tio;  /* saved terminal settings */

/* ---------------------------------------------------------------
 * keyboard_init: switch terminal to raw + non-canonical mode.
 * Characters are available immediately (no Enter required),
 * echo is disabled, and stdin becomes non-blocking.
 * --------------------------------------------------------------- */
void keyboard_init(void) {
    struct termios new_tio;

    /* Save current terminal settings */
    tcgetattr(STDIN_FILENO, &old_tio);
    new_tio = old_tio;

    /* Raw mode: no line buffering, no echo */
    new_tio.c_lflag &= ~(ICANON | ECHO);

    /* Minimum chars = 0, timeout = 0 → fully non-blocking */
    new_tio.c_cc[VMIN]  = 0;
    new_tio.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

    /* Also set stdin fd to O_NONBLOCK so read() returns immediately */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

/* ---------------------------------------------------------------
 * keyboard_restore: put the terminal back to its original state.
 * Called on exit / game-over to avoid leaving terminal broken.
 * --------------------------------------------------------------- */
void keyboard_restore(void) {
    /* Remove O_NONBLOCK */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    /* Restore original termios settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
}

/* ---------------------------------------------------------------
 * keyPressed: non-blocking read.
 * Returns the character pressed, OR 0 if no key was pressed.
 * Also handles ANSI escape sequences for arrow keys and maps
 * them to WASD equivalents.
 * --------------------------------------------------------------- */
char keyPressed(void) {
    char c = 0;
    int  n = (int)read(STDIN_FILENO, &c, 1);
    if (n <= 0) return 0;

    /* Arrow key: ESC [ A/B/C/D  (~3 bytes) */
    if (c == '\033') {
        char seq[2];
        /* Try to read two more bytes; they may or may not be there */
        if (read(STDIN_FILENO, &seq[0], 1) <= 0) return c;
        if (read(STDIN_FILENO, &seq[1], 1) <= 0) return c;
        if (seq[0] == '[') {
            if (seq[1] == 'A') return KEY_UP;     /* ↑ */
            if (seq[1] == 'B') return KEY_DOWN;   /* ↓ */
            if (seq[1] == 'C') return KEY_RIGHT;  /* → */
            if (seq[1] == 'D') return KEY_LEFT;   /* ← */
        }
        return 0;
    }

    return c;
}

/* ---------------------------------------------------------------
 * readLine: blocking line reader.
 * Reads characters one by one until '\n' or buffer is full.
 * Used in the start/game-over screens.
 * --------------------------------------------------------------- */
void readLine(char *buf, int max_len) {
    /* Re-enable canonical mode for this call */
    struct termios tmp;
    tcgetattr(STDIN_FILENO, &tmp);
    tmp.c_lflag |= (ICANON | ECHO);
    tmp.c_cc[VMIN]  = 1;
    tmp.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &tmp);

    /* Remove O_NONBLOCK so read blocks */
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK);

    int i = 0;
    while (i < max_len - 1) {
        char c = 0;
        if (read(STDIN_FILENO, &c, 1) <= 0) break;
        if (c == '\n') break;
        buf[i++] = c;
    }
    buf[i] = '\0';

    /* Restore raw/non-blocking for the game loop */
    keyboard_init();
}
