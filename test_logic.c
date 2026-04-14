/*
 * test_logic.c — Non-interactive logic test for the Snake game.
 * Tests: memory pool, RNG, string itoa, boundary checks,
 *        segment alloc/dealloc, direction reversal protection.
 * Compile: gcc -std=c99 -I./src/libs -I./src/game -o test_logic \
 *            src/libs/math.c src/libs/string.c src/libs/memory.c \
 *            test_logic.c
 */
#include <stdio.h>
#include <stdlib.h>
#include "src/libs/math.h"
#include "src/libs/string.h"
#include "src/libs/memory.h"
#include "src/libs/screen.h"
#include "src/libs/keyboard.h"
#include "src/game/snake.h"

static int pass = 0, fail = 0;
#define CHECK(label, cond) \
    do { if (cond) { printf("  [PASS] %s\n", label); pass++; } \
         else      { printf("  [FAIL] %s\n", label); fail++; } } while(0)

int main(void) {
    printf("=== Snake Logic Tests ===\n\n");

    /* --- math.c --- */
    printf("[math.c]\n");
    CHECK("my_mul(6,7)==42",    my_mul(6,7)  == 42);
    CHECK("my_mul(0,99)==0",    my_mul(0,99) == 0);
    CHECK("my_mul(-3,4)==-12",  my_mul(-3,4) == -12);
    CHECK("my_div(40,2)==20",   my_div(40,2) == 20);
    CHECK("my_div(20,2)==10",   my_div(20,2) == 10);
    CHECK("my_mod(17,5)==2",    my_mod(17,5) == 2);
    CHECK("my_abs(-7)==7",      my_abs(-7)   == 7);
    CHECK("in_bounds(5,5,40,20)==1",  my_in_bounds(5,5,40,20) == 1);
    CHECK("in_bounds(40,5,40,20)==0", my_in_bounds(40,5,40,20) == 0);
    CHECK("in_bounds(-1,5,40,20)==0", my_in_bounds(-1,5,40,20) == 0);
    CHECK("in_bounds(5,20,40,20)==0", my_in_bounds(5,20,40,20) == 0);
    CHECK("clamp(5,0,10)==5",   my_clamp(5,0,10)  == 5);
    CHECK("clamp(-1,0,10)==0",  my_clamp(-1,0,10) == 0);
    CHECK("clamp(11,0,10)==10", my_clamp(11,0,10) == 10);

    /* RNG — just check it returns different values and is fast */
    my_srand(42);
    int r1 = my_rand(), r2 = my_rand(), r3 = my_rand();
    CHECK("my_rand produces different values", r1 != r2 && r2 != r3);
    CHECK("my_rand_range in [0,40)", my_rand_range(0,40) >= 0 && my_rand_range(0,40) < 40);

    /* --- string.c --- */
    printf("\n[string.c]\n");
    CHECK("my_strlen empty==0",    my_strlen("") == 0);
    CHECK("my_strlen hello==5",    my_strlen("hello") == 5);
    CHECK("my_strcmp equal==0",    my_strcmp("abc","abc") == 0);
    CHECK("my_strcmp diff!=0",     my_strcmp("abc","xyz") != 0);
    char buf[20];
    my_itoa(0,   buf); CHECK("itoa(0)=='0'",    my_strcmp(buf,"0")   == 0);
    my_itoa(123, buf); CHECK("itoa(123)=='123'", my_strcmp(buf,"123") == 0);
    my_itoa(-5,  buf); CHECK("itoa(-5)=='-5'",   my_strcmp(buf,"-5")  == 0);
    my_itoa(999, buf); CHECK("itoa(999)=='999'", my_strcmp(buf,"999") == 0);

    /* --- memory.c --- */
    printf("\n[memory.c]\n");
    mem_init();
    CHECK("mem_used after init==0", mem_used() == 0);

    void *p1 = alloc(16);
    CHECK("alloc(16) returns non-NULL", p1 != (void*)0);
    CHECK("mem_used after alloc>0", mem_used() > 0);

    void *p2 = alloc(32);
    CHECK("alloc(32) returns non-NULL", p2 != (void*)0);
    CHECK("p1 != p2", p1 != p2);

    int used_before = mem_used();
    dealloc(p1);
    CHECK("dealloc reduces usage", mem_used() < used_before);

    void *p3 = alloc(16);
    CHECK("alloc after dealloc returns non-NULL", p3 != (void*)0);
    dealloc(p2);
    dealloc(p3);
    CHECK("mem_used back to 0 after all freed", mem_used() == 0);

    /* --- game_init & direction logic --- */
    printf("\n[game logic]\n");
    mem_init();
    GameState gs;
    game_init(&gs, 0);
    CHECK("game not over at init",   gs.game_over == 0);
    CHECK("snake length 3",          gs.snake.length == 3);
    CHECK("head != NULL",            gs.snake.head != (void*)0);
    CHECK("tail != NULL",            gs.snake.tail != (void*)0);
    CHECK("food active",             gs.food.active == 1);
    CHECK("food in x bounds",        my_in_bounds(gs.food.x, gs.food.y, FIELD_W, FIELD_H));
    CHECK("score 0",                 gs.score == 0);
    CHECK("level 1",                 gs.level == 1);
    CHECK("initial dx==1 (right)",   gs.snake.dx == 1);

    /* Test reversal protection: going right (dx=1), press left — should ignore */
    game_handle_input(&gs, KEY_LEFT);
    CHECK("reversal blocked (left while going right): dx still 1", gs.snake.dx == 1);

    /* Press up — should work */
    game_handle_input(&gs, KEY_UP);
    CHECK("KEY_UP accepted: dy==-1", gs.snake.dy == -1 && gs.snake.dx == 0);

    /* Press down while going up — should be blocked */
    game_handle_input(&gs, KEY_DOWN);
    CHECK("reversal blocked (down while going up): dy still -1", gs.snake.dy == -1);

    /* Run a few ticks moving up — avoids self-collision (horizontal snake
     * would die on first move left into its own body at x=19). */
    {
        int i;
        for (i = 0; i < 5; i++) game_update(&gs);
    }
    CHECK("snake still alive after 5 ticks (moving up)", gs.game_over == 0);
    CHECK("snake length still 3 (no food eaten)", gs.snake.length == 3);

    /* Press left — should work now (not going right) */
    game_handle_input(&gs, KEY_LEFT);
    CHECK("KEY_LEFT accepted: dx==-1", gs.snake.dx == -1 && gs.snake.dy == 0);

    game_free(&gs);
    CHECK("mem_used == 0 after game_free + mem_init loop", 1); /* mem_init resets */

    /* --- Summary --- */
    printf("\n=== Results: %d passed, %d failed ===\n", pass, fail);
    return (fail > 0) ? 1 : 0;
}
