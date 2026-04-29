/*
 * sound.c — Native terminal sound stubs.
 * Uses the terminal bell (\a) for audible feedback where meaningful.
 * All real synthesis lives in sound_wasm.c for the browser build.
 */
#include "sound.h"
#include <stdio.h>   /* putchar — allowed by project rules */

void sound_eat(int combo)   { (void)combo; putchar('\a'); }
void sound_die(void)        { putchar('\a'); }
void sound_levelup(void)    { putchar('\a'); }
void sound_game_start(void) { putchar('\a'); }
void sound_wrap(void)       { /* bell on every wrap would be noise */ }
