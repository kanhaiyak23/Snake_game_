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
static int esc_state = 0;       /* 0:none, 1:ESC, 2:ESC+[ , 3:ESC+O */

/* ---------------------------------------------------------------
 * keyboard_init: switch terminal to raw + non-canonical mode. for real-time input
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
    char last_key = 0;
    while (read(STDIN_FILENO, &c, 1) > 0) {
        if (esc_state == 0) {
            if (c == '\033') { esc_state = 1; continue; }
            if (c >= 'A' && c <= 'Z') c = (char)(c + ('a' - 'A'));
            last_key = c;
            continue;
        }

        if (esc_state == 1) {
            if (c == '[') {
                esc_state = 2;
                continue;
            }
            /* Some terminals emit ESC O A/B/C/D for arrows. */
            if (c == 'O') {
                esc_state = 3;
                continue;
            }
            esc_state = 0;
            if (c == '\033') { esc_state = 1; continue; }
            continue;
        }

        /* esc_state == 2 or 3 -> expect final arrow byte */
        if (esc_state == 2 || esc_state == 3) {
            esc_state = 0;
            if (c == 'A') { last_key = KEY_UP;    continue; }  /* ↑ */
            if (c == 'B') { last_key = KEY_DOWN;  continue; }  /* ↓ */
            if (c == 'C') { last_key = KEY_RIGHT; continue; }  /* → */
            if (c == 'D') { last_key = KEY_LEFT;  continue; }  /* ← */
            continue;
        }

        esc_state = 0;
    }
    return last_key;
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
