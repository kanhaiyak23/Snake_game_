/*
 * keyboard_wasm.c — Browser keyboard implementation for Emscripten.
 *
 * Replaces keyboard.c entirely for the Wasm build.
 * JS calls wasm_push_key(keyCode) on keydown events;
 * the game loop reads it via keyPressed().
 */
#include "../libs/keyboard.h"
#include <emscripten.h>

static char key_buf = 0;

/* Called from JavaScript on every keydown event. */
EMSCRIPTEN_KEEPALIVE
void wasm_push_key(int js_keycode) {
    switch (js_keycode) {
        case 87: case 38: key_buf = KEY_UP;    break; /* W / ArrowUp    */
        case 83: case 40: key_buf = KEY_DOWN;  break; /* S / ArrowDown  */
        case 65: case 37: key_buf = KEY_LEFT;  break; /* A / ArrowLeft  */
        case 68: case 39: key_buf = KEY_RIGHT; break; /* D / ArrowRight */
        case 80:          key_buf = KEY_PAUSE; break; /* P              */
        case 81:          key_buf = KEY_QUIT;  break; /* Q              */
        case 32:          key_buf = ' ';       break; /* Space          */
        case 13:          key_buf = '\r';      break; /* Enter          */
        default: break;
    }
}

void keyboard_init(void)    { /* no-op: browser handles raw input */ }
void keyboard_restore(void) { /* no-op: nothing to restore        */ }

char keyPressed(void) {
    char k  = key_buf;
    key_buf = 0;
    return k;
}

void readLine(char *buf, int max_len) {
    if (max_len > 0) buf[0] = '\0';
}
