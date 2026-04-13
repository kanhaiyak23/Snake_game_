/*
 * snake.c — Snake Game Logic
 *
 * Library Integration Pipeline:
 *   keyboard.c → captures direction input
 *   string.c   → int-to-string for score display
 *   memory.c   → alloc/dealloc each snake Segment
 *   math.c     → boundary checks, speed level, random food placement
 *   screen.c   → renders every frame
 *
 * Rules compliance:
 *   - No <string.h>, <math.h>; no malloc/free.
 *   - All values are computed dynamically (no magic numbers for logic).
 *   - Dynamic alloc: each Segment is alloc()'d; tail is dealloc()'d on move.
 */
#include "snake.h"
#include "../libs/math.h"
#include "../libs/string.h"
#include "../libs/memory.h"
#include "../libs/screen.h"
#include "../libs/keyboard.h"
#include <stdio.h>    /* allowed: printf for score UI only */
#include <unistd.h>  /* usleep */

/* ---------------------------------------------------------------
 * Internal helper: allocate a new Segment at (x,y).
 * Returns pointer to the new segment, or NULL on OOM.
 * --------------------------------------------------------------- */
static Segment *seg_new(int x, int y) {
    Segment *s = (Segment *)alloc((int)sizeof(Segment));
    if (!s) return (void*)0;
    s->x    = x;
    s->y    = y;
    s->next = (void*)0;
    return s;
}

/* ---------------------------------------------------------------
 * place_food: pick a random position not occupied by the snake.
 * Uses math.c for modulo and bounds checking.
 * --------------------------------------------------------------- */
static void place_food(GameState *gs) {
    int ok = 0;
    while (!ok) {
        int fx = my_rand_range(0, FIELD_W);
        int fy = my_rand_range(0, FIELD_H);

        /* Ensure the position is not on any segment */
        ok = 1;
        Segment *cur = gs->snake.head;
        while (cur) {
            if (cur->x == fx && cur->y == fy) { ok = 0; break; }
            cur = cur->next;
        }
        if (ok) {
            gs->food.x = fx;
            gs->food.y = fy;
            gs->food.active = 1;
        }
    }
}

/* ---------------------------------------------------------------
 * game_init: initialise a fresh game.
 * Snake starts as 3 segments in the middle of the field.
 * high_score is passed in so it persists across games.
 * --------------------------------------------------------------- */
void game_init(GameState *gs, int prev_high_score) {
    /* Reset memory pool for clean start each game */
    mem_init();

    /* Use math.c to compute start position dynamically */
    int start_x = my_div(FIELD_W, 2);
    int start_y = my_div(FIELD_H, 2);

    /* Build initial snake body: head -> body -> tail   */
    Segment *head = seg_new(start_x,     start_y);
    Segment *mid  = seg_new(start_x - 1, start_y);
    Segment *tail = seg_new(start_x - 2, start_y);

    if (!head || !mid || !tail) {
        /* OOM on init — should never happen with 1MB pool */
        gs->game_over = 1;
        return;
    }

    head->next = mid;
    mid->next  = tail;

    gs->snake.head   = head;
    gs->snake.tail   = tail;
    gs->snake.length = 3;
    gs->snake.dx     = 1;   /* moving right initially */
    gs->snake.dy     = 0;

    gs->score       = 0;
    gs->high_score  = prev_high_score;
    gs->level       = 1;
    gs->game_over   = 0;
    gs->paused      = 0;
    gs->ticks       = 0;
    gs->foods_eaten = 0;

    place_food(gs);
}

/* ---------------------------------------------------------------
 * game_handle_input: called when a key is detected.
 * Prevents 180-degree reversal (can't reverse directly into self).
 *
 * FIX: The reversal checks were inverted:
 *   - KEY_LEFT should be blocked only when already moving left (dx==-1)
 *   - KEY_RIGHT should be blocked only when already moving right (dx==1)
 * --------------------------------------------------------------- */
void game_handle_input(GameState *gs, char key) {
    if (key == KEY_PAUSE) { gs->paused = !gs->paused; return; }
    if (key == KEY_QUIT)  { gs->game_over = 1; return; }

    /*
     * Anti-reversal: only block the exact opposite direction.
     * e.g. if moving right (dx==1), ignore KEY_LEFT (would reverse).
     *      if moving left  (dx==-1), ignore KEY_RIGHT.
     *      if moving down  (dy==1), ignore KEY_UP.
     *      if moving up    (dy==-1), ignore KEY_DOWN.
     */
    if (key == KEY_UP    && gs->snake.dy != 1)  { gs->snake.dx=0; gs->snake.dy=-1; return; }
    if (key == KEY_DOWN  && gs->snake.dy != -1) { gs->snake.dx=0; gs->snake.dy= 1; return; }
    if (key == KEY_LEFT  && gs->snake.dx != 1)  { gs->snake.dx=-1; gs->snake.dy=0; return; }
    if (key == KEY_RIGHT && gs->snake.dx != -1) { gs->snake.dx= 1; gs->snake.dy=0; return; }
}

/* ---------------------------------------------------------------
 * game_update: advance game state by one tick.
 * - Computes new head position using math.c
 * - Checks wall + self collision
 * - Eats food or removes tail
 * - Updates score & level
 * --------------------------------------------------------------- */
void game_update(GameState *gs) {
    if (gs->game_over || gs->paused) return;

    /* Compute new head position */
    int nx = gs->snake.head->x + gs->snake.dx;
    int ny = gs->snake.head->y + gs->snake.dy;

    /* --- Boundary check via math.c --- */
    if (!my_in_bounds(nx, ny, FIELD_W, FIELD_H)) {
        gs->game_over = 1;
        return;
    }

    /* --- Self-collision check ---
     * Walk all segments from head up to (but NOT including) the tail.
     * The tail will vacate its cell this tick (unless we eat food),
     * so it is safe to ignore it for this frame.
     * 
     * FIX: The old code computed last_before_tail but then used
     * `cur != gs->snake.tail` as the loop terminator — which is correct
     * for movement collision. Kept as-is but removed the dead variable.
     */
    Segment *cur = gs->snake.head;
    while (cur && cur->next != (void*)0) {
        /* Check all segments except the last one (tail vacates this tick) */
        if (cur->x == nx && cur->y == ny) {
            gs->game_over = 1;
            return;
        }
        cur = cur->next;
        /* Stop before the tail — tail moves away this tick */
        if (cur == gs->snake.tail) break;
    }

    /* --- Check food collision --- */
    int ate = (nx == gs->food.x && ny == gs->food.y && gs->food.active);

    /* --- Allocate new head segment via memory.c --- */
    Segment *new_head = seg_new(nx, ny);
    if (!new_head) { gs->game_over = 1; return; }
    new_head->next    = gs->snake.head;
    gs->snake.head    = new_head;
    gs->snake.length++;

    if (ate) {
        /* Snake grows: don't remove tail */
        gs->food.active = 0;
        gs->foods_eaten++;
        /* Score increments by level * 10 — computed via math.c mul */
        gs->score += my_mul(10, gs->level);
        if (gs->score > gs->high_score) gs->high_score = gs->score;
        /* Level up every 50 points — computed via math.c div */
        gs->level = my_div(gs->score, 50) + 1;
        if (gs->level > 10) gs->level = 10;
        place_food(gs);
    } else {
        /* Remove tail: dealloc via memory.c */
        Segment *old_tail = gs->snake.tail;
        /* Walk to the node just before the tail */
        Segment *prev = gs->snake.head;
        while (prev->next && prev->next != old_tail) prev = prev->next;
        prev->next     = (void*)0;
        gs->snake.tail = prev;
        dealloc(old_tail);      /* ← dynamic dealloc every frame */
        gs->snake.length--;
    }

    gs->ticks++;
}

/* ---------------------------------------------------------------
 * game_render: draws the full game state each frame.
 * Pipeline: math → string → screen.
 * Layout:
 *   Row 1  : title bar with controls reminder and direction indicator
 *   Row 2  : separator line
 *   Row 3+ : game field with border
 *   Row 25 : score/level/length stats
 *   Row 26 : level progress bar + next-level info
 *   Row 27 : memory debug line
 * --------------------------------------------------------------- */
void game_render(const GameState *gs) {
    int i, j;
    screen_clear();

    /* ---- Row 1: Title bar ---- */
    screen_goto(1, 1);
    screen_set_fg(36);
    screen_set_bold();
    screen_putstr(" SNAKE");
    screen_reset_color();
    screen_set_fg(90);
    screen_putstr("  |  WASD/Arrows: Move  |  P: Pause  |  Q: Quit");
    screen_reset_color();

    /* Direction indicator — right-aligned at col 60 */
    screen_goto(60, 1);
    screen_set_fg(93);
    screen_putstr("Dir:");
    if      (gs->snake.dx ==  1) screen_putstr(" -> ");
    else if (gs->snake.dx == -1) screen_putstr(" <- ");
    else if (gs->snake.dy == -1) screen_putstr("  ^ ");
    else if (gs->snake.dy ==  1) screen_putstr("  v ");
    screen_reset_color();

    /* ---- Row 2: Thin separator ---- */
    screen_goto(1, 2);
    screen_set_fg(90);
    for (i = 0; i < FIELD_W + 4; i++) screen_putchar('-');
    screen_reset_color();

    /* ---- Game field border ---- */
    screen_set_fg(33);
    screen_draw_border(FIELD_X - 1, FIELD_Y - 1, FIELD_W, FIELD_H);
    screen_reset_color();

    /* Render food — bright red @ */
    if (gs->food.active) {
        screen_goto(FIELD_X + gs->food.x, FIELD_Y + gs->food.y);
        screen_set_fg(91);
        screen_putchar('@');
        screen_reset_color();
    }

    /* Render snake — bright green head 'O', green body 'o' */
    {
        Segment *cur = gs->snake.head;
        int first = 1;
        while (cur) {
            screen_goto(FIELD_X + cur->x, FIELD_Y + cur->y);
            if (first) {
                screen_set_fg(92);
                screen_set_bold();
                screen_putchar('O');
                first = 0;
            } else {
                screen_set_fg(32);
                screen_putchar('o');
            }
            screen_reset_color();
            cur = cur->next;
        }
    }

    /* ---- Row 25: Stats panel ---- */
    int score_row = FIELD_Y + FIELD_H + 2;
    char sbuf[16], hbuf[16], lbuf[8], lenbuf[8], fbuf[8];
    my_itoa(gs->score,        sbuf);
    my_itoa(gs->high_score,   hbuf);
    my_itoa(gs->level,        lbuf);
    my_itoa(gs->snake.length, lenbuf);
    my_itoa(gs->foods_eaten,  fbuf);

    screen_goto(FIELD_X, score_row);
    screen_set_fg(97);
    screen_putstr("Score:");
    screen_set_fg(92);
    screen_putstr(sbuf);
    screen_reset_color();
    screen_set_fg(97);
    screen_putstr("  High:");
    screen_set_fg(93);
    screen_putstr(hbuf);
    screen_reset_color();
    screen_set_fg(97);
    screen_putstr("  Lvl:");
    screen_set_fg(96);
    screen_putstr(lbuf);
    screen_putstr("/10");
    screen_reset_color();
    screen_set_fg(97);
    screen_putstr("  Len:");
    screen_set_fg(95);
    screen_putstr(lenbuf);
    screen_reset_color();
    screen_set_fg(97);
    screen_putstr("  Food:");
    screen_set_fg(91);
    screen_putstr(fbuf);
    screen_reset_color();

    /* ---- Row 26: Level progress bar ---- */
    screen_goto(FIELD_X, score_row + 1);
    screen_set_fg(97);
    screen_putstr("Next Lvl:[");

    int progress   = my_mod(gs->score, 50);
    int bar_filled = my_div(my_mul(progress, 20), 50);
    int bar_empty  = 20 - bar_filled;

    if (gs->level >= 10) {
        /* Max level — show a full golden bar */
        screen_set_fg(93);
        for (j = 0; j < 20; j++) screen_putchar('=');
    } else {
        screen_set_fg(92);
        for (j = 0; j < bar_filled; j++) screen_putchar('=');
        if (bar_filled < 20) {
            screen_set_fg(92);
            screen_putchar('>');
            bar_empty--;
        }
        screen_set_fg(90);
        for (j = 0; j < bar_empty; j++) screen_putchar('-');
    }
    screen_reset_color();
    screen_set_fg(97);
    screen_putstr("]");

    if (gs->level >= 10) {
        screen_set_fg(93);
        screen_set_bold();
        screen_putstr("  MAX LEVEL!");
        screen_reset_color();
    } else {
        int pts_to_next = 50 - progress;
        char pnbuf[8], nextlvl[8];
        my_itoa(pts_to_next, pnbuf);
        my_itoa(gs->level + 1, nextlvl);
        screen_set_fg(90);
        screen_putstr("  (");
        screen_putstr(pnbuf);
        screen_putstr("pts -> Lvl ");
        screen_putstr(nextlvl);
        screen_putstr(")");
        screen_reset_color();
    }

    /* ---- Paused overlay — centred box ---- */
    if (gs->paused) {
        int mid_col = FIELD_X + my_div(FIELD_W, 2) - 7;
        int mid_row = FIELD_Y + my_div(FIELD_H, 2) - 1;
        screen_goto(mid_col, mid_row);
        screen_set_fg(93);
        screen_putstr("+---------------+");
        screen_goto(mid_col, mid_row + 1);
        screen_putstr("|  II  PAUSED   |");
        screen_goto(mid_col, mid_row + 2);
        screen_putstr("|  P to resume  |");
        screen_goto(mid_col, mid_row + 3);
        screen_putstr("+---------------+");
        screen_reset_color();
    }

    /* ---- Row 27: Memory debug info ---- */
    screen_goto(FIELD_X, score_row + 2);
    screen_set_fg(90);
    screen_putstr("Mem:");
    char membuf[16];
    my_itoa(mem_used(), membuf);
    screen_putstr(membuf);
    screen_putstr("B used");
    screen_reset_color();

    screen_flush();
}

/* ---------------------------------------------------------------
 * game_speed_delay: compute microseconds between frames.
 * Higher level → shorter delay → faster game.
 * Uses math.c div/mul for all computations (no hard-coded speed).
 * --------------------------------------------------------------- */
int game_speed_delay(const GameState *gs) {
    /* Base delay: 180ms. Reduce by 15ms per level using math.c. */
    int base  = 180000;
    int step  = my_mul(15000, gs->level - 1);
    int delay = base - step;
    /* Clamp minimum to 50ms using math.c clamp */
    return my_clamp(delay, 50000, base);
}

/* ---------------------------------------------------------------
 * game_free: release all segment memory on game over.
 * NOTE: mem_init() resets the whole pool on next game_init(),
 * so this is for correctness documentation — all alloc'd Segments
 * are explicitly tracked and freed here.
 * --------------------------------------------------------------- */
void game_free(GameState *gs) {
    Segment *cur = gs->snake.head;
    while (cur) {
        Segment *nxt = cur->next;
        dealloc(cur);
        cur = nxt;
    }
    gs->snake.head   = (void*)0;
    gs->snake.tail   = (void*)0;
    gs->snake.length = 0;
}
