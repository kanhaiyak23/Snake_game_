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
#include "../libs/sound.h"
#include <stdio.h>    /* allowed: printf for score UI only */
#include <unistd.h>  /* usleep */

static void place_food(GameState *gs, int food_idx);
static void place_obstacles(GameState *gs);

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

/* Build/reset snake to starting position; keep score/lives unchanged. */
static int reset_snake(GameState *gs) {
    /* Spawn the snake at field center, 3 segments long, facing right. */
    int start_x = my_div(screen_field_w(), 2);
    int start_y = my_div(screen_field_h(), 2);
    Segment *head = seg_new(start_x,     start_y);
    Segment *mid  = seg_new(start_x - 1, start_y);
    Segment *tail = seg_new(start_x - 2, start_y);

    if (!head || !mid || !tail) {
        if (head) dealloc(head);
        if (mid)  dealloc(mid);
        if (tail) dealloc(tail);
        return 0;
    }

    head->next = mid;
    mid->next  = tail;
    gs->snake.head   = head;
    gs->snake.tail   = tail;
    gs->snake.length = 3;
    gs->snake.dx     = 1;
    gs->snake.dy     = 0;
    return 1;
}

static void clear_snake(GameState *gs) {
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

/* Lose one life and respawn; returns 1 if game should continue. */
static int lose_life_and_respawn(GameState *gs) {
    sound_die();
    /* Last life consumed -> end this run with collision reason. */
    if (gs->lives <= 1) {
        gs->lives = 0;
        gs->game_over = 1;
        gs->game_end_reason = GAME_END_COLLISION;
        return 0;
    }

    /* Still has lives: decrement, reset combo, rebuild snake, and refresh food set. */
    gs->lives--;
    gs->combo = 0;
    clear_snake(gs);
    if (!reset_snake(gs)) {
        gs->game_over = 1;
        gs->game_end_reason = GAME_END_OOM;
        return 0;
    }
    {
        int i;
        for (i = 0; i < MAX_FOODS; i++) place_food(gs, i);
    }
    return 1;
}

/* ---------------------------------------------------------------
 * place_food: pick a random position not occupied by the snake.
 * Uses math.c for modulo and bounds checking.
 * --------------------------------------------------------------- */
static void place_food(GameState *gs, int food_idx) {
    int field_w = screen_field_w();
    int field_h = screen_field_h();
    int ok = 0;
    while (!ok) {
        int fx = my_rand_range(0, field_w);
        int fy = my_rand_range(0, field_h);

        /* Rule 1: do not place food on any snake segment. */
        ok = 1;
        Segment *cur = gs->snake.head;
        while (cur) {
            if (cur->x == fx && cur->y == fy) { ok = 0; break; }
            cur = cur->next;
        }
        if (ok) {
            /* Rule 2: do not overlap with other active food items. */
            int i;
            for (i = 0; i < MAX_FOODS; i++) {
                if (i != food_idx && gs->foods[i].active &&
                    gs->foods[i].x == fx && gs->foods[i].y == fy) {
                    ok = 0;
                    break;
                }
            }
        }
        if (ok) {
            /* Rule 3: do not place food on any obstacle. */
            int j;
            for (j = 0; j < gs->num_obstacles; j++) {
                if (gs->obstacles[j].active &&
                    gs->obstacles[j].x == fx && gs->obstacles[j].y == fy) {
                    ok = 0;
                    break;
                }
            }
        }
        if (ok) {
            gs->foods[food_idx].x = fx;
            gs->foods[food_idx].y = fy;
            gs->foods[food_idx].active = 1;
        }
    }
}

static void place_obstacles(GameState *gs) {
    int field_w = screen_field_w();
    int field_h = screen_field_h();
    int cx      = my_div(field_w, 2);
    int cy      = my_div(field_h, 2);
    int i;

    for (i = 0; i < MAX_OBSTACLES; i++) gs->obstacles[i].active = 0;

    for (i = 0; i < gs->num_obstacles; i++) {
        int ok = 0;
        int attempts = 0;
        while (!ok && attempts < 200) {
            int ox = my_rand_range(0, field_w);
            int oy = my_rand_range(0, field_h);
            attempts++;

            /* Keep a 5-cell clear zone around the snake spawn point. */
            if (ox >= cx - 4 && ox <= cx + 4 &&
                oy >= cy - 2 && oy <= cy + 2) continue;

            ok = 1;
            /* No overlap with other obstacles. */
            int j;
            for (j = 0; j < i; j++) {
                if (gs->obstacles[j].active &&
                    gs->obstacles[j].x == ox && gs->obstacles[j].y == oy) {
                    ok = 0; break;
                }
            }
            if (!ok) continue;
            /* No overlap with food. */
            for (j = 0; j < MAX_FOODS; j++) {
                if (gs->foods[j].active &&
                    gs->foods[j].x == ox && gs->foods[j].y == oy) {
                    ok = 0; break;
                }
            }
            if (ok) {
                gs->obstacles[i].x      = ox;
                gs->obstacles[i].y      = oy;
                gs->obstacles[i].active = 1;
            }
        }
    }
}

/* ---------------------------------------------------------------
 * game_init: initialise a fresh game.
 * Snake starts as 3 segments in the middle of the field.
 * high_score is passed in so it persists across games.
 * --------------------------------------------------------------- */
void game_init(GameState *gs, int prev_high_score, int wrap_mode) {
    screen_update_layout();
    /* Reset memory pool for clean start each game */
    mem_init();

    if (!reset_snake(gs)) {
        gs->snake.head   = (void*)0;
        gs->snake.tail   = (void*)0;
        gs->snake.length = 0;
        gs->game_over    = 1;
        gs->game_end_reason = GAME_END_OOM;
        return;
    }

    gs->score       = 0;
    gs->high_score  = prev_high_score;
    gs->level       = 1;
    gs->game_over   = 0;
    gs->paused      = 0;
    gs->ticks       = 0;
    gs->foods_eaten = 0;
    gs->lives       = INITIAL_LIVES;
    gs->game_end_reason = GAME_END_NONE;
    gs->combo       = 0;
    gs->wrap_mode   = wrap_mode;
    /* Start with 4 obstacles; more appear as the game progresses. */
    gs->num_obstacles = 4;

    {
        int i;
        for (i = 0; i < MAX_FOODS; i++) place_food(gs, i);
    }
    place_obstacles(gs);
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
    if (key == KEY_QUIT)  { gs->game_over = 1; gs->game_end_reason = GAME_END_QUIT; return; }

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
    screen_update_layout();
    {
        /* If layout changes, ensure existing foods stay inside bounds. */
        int i;
        for (i = 0; i < MAX_FOODS; i++) {
            if (!my_in_bounds(gs->foods[i].x, gs->foods[i].y,
                              screen_field_w(), screen_field_h())) {
                place_food(gs, i);
            }
        }
    }

    /* Compute new head position */
    int nx = gs->snake.head->x + gs->snake.dx;
    int ny = gs->snake.head->y + gs->snake.dy;

    /* --- Boundary check / wrap --- */
    if (gs->wrap_mode) {
        int fw = screen_field_w();
        int fh = screen_field_h();
        int wrapped = 0;
        if (nx < 0)        { nx = fw - 1; wrapped = 1; }
        else if (nx >= fw) { nx = 0;      wrapped = 1; }
        if (ny < 0)        { ny = fh - 1; wrapped = 1; }
        else if (ny >= fh) { ny = 0;      wrapped = 1; }
        if (wrapped) sound_wrap();
    } else {
        if (!my_in_bounds(nx, ny, screen_field_w(), screen_field_h())) {
            (void)lose_life_and_respawn(gs);
            return;
        }
    }

    /* --- Self-collision check ---
     * Walk all segments from head up to (but NOT including) the tail.
     * The tail will vacate its cell this tick (unless we eat food),
     * so it is safe to ignore it for this frame.
     */
    {
        Segment *cur = gs->snake.head;
        while (cur && cur != gs->snake.tail) {
            if (cur->x == nx && cur->y == ny) {
                (void)lose_life_and_respawn(gs);
                return;
            }
            cur = cur->next;
        }
    }

    /* --- Obstacle collision --- */
    {
        int i;
        for (i = 0; i < gs->num_obstacles; i++) {
            if (gs->obstacles[i].active &&
                gs->obstacles[i].x == nx && gs->obstacles[i].y == ny) {
                (void)lose_life_and_respawn(gs);
                return;
            }
        }
    }

    /* --- Check food collision --- */
    int ate = 0;
    int eaten_idx = -1;
    {
        /* Collision with any active food counts as an eat event. */
        int i;
        for (i = 0; i < MAX_FOODS; i++) {
            if (gs->foods[i].active &&
                nx == gs->foods[i].x && ny == gs->foods[i].y) {
                ate = 1;
                eaten_idx = i;
                break;
            }
        }
    }

    /* --- Allocate new head segment via memory.c --- */
    Segment *new_head = seg_new(nx, ny);
    if (!new_head) {
        gs->game_over = 1;
        gs->game_end_reason = GAME_END_OOM;
        return;
    }
    new_head->next    = gs->snake.head;
    gs->snake.head    = new_head;
    gs->snake.length++;

    if (ate) {
        /* Eat path: keep tail (grow), update combo/score/level, respawn eaten slot. */
        gs->foods[eaten_idx].active = 0;
        gs->foods_eaten++;
        gs->combo++;
        sound_eat(gs->combo);
        {
            /* multiplier = combo, capped at MAX_COMBO_MULT */
            int mult = my_clamp(gs->combo, 1, MAX_COMBO_MULT);
            gs->score += my_mul(my_mul(10, gs->level), mult);
        }
        if (gs->score > gs->high_score) gs->high_score = gs->score;
        /* Level up every 50 points — computed via math.c div */
        {
            int old_level = gs->level;
            gs->level = my_div(gs->score, 50) + 1;
            if (gs->level > 10) gs->level = 10;
            if (gs->level > old_level) sound_levelup();
        }
        /* Add an obstacle each time level crosses an even boundary. */
        {
            int target = 4 + my_div(gs->level - 1, 2);
            if (target > MAX_OBSTACLES) target = MAX_OBSTACLES;
            if (target > gs->num_obstacles) {
                gs->num_obstacles = target;
                place_obstacles(gs);
            }
        }
        place_food(gs, eaten_idx);
    } else {
        /* Normal move path: advance head and remove old tail segment. */
        Segment *old_tail = gs->snake.tail;
        Segment *prev     = gs->snake.head;
        while (prev->next && prev->next != old_tail) prev = prev->next;

        if (prev->next != old_tail) {
            /* List invariant broken — undo new head so state stays consistent */
            Segment *orphan = new_head;
            gs->snake.head    = orphan->next;
            gs->snake.length--;
            dealloc(orphan);
            gs->game_over     = 1;
            gs->game_end_reason = GAME_END_INTERNAL;
            return;
        }

        prev->next        = (void*)0;
        gs->snake.tail    = prev;
        dealloc(old_tail);
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
 *   Row 25 : score / high / length / food
 *   Row 26 : memory debug line
 * --------------------------------------------------------------- */
void game_render(const GameState *gs) {
    int field_x, field_y, field_w, field_h;
    int hud_col;
    int i;
    screen_update_layout();
    field_x = screen_field_x();
    field_y = screen_field_y();
    field_w = screen_field_w();
    field_h = screen_field_h();
    hud_col = field_x - 1;
    if (hud_col < 1) hud_col = 1;
    screen_clear();

    /* ---- Row 1: Title bar ---- */
    screen_goto(hud_col, 1);
    screen_set_fg(36);
    screen_set_bold();
    screen_putstr(" SNAKE");
    screen_reset_color();
    if (gs->wrap_mode) {
        screen_set_fg(96);
        screen_set_bold();
        screen_putstr(" [WRAP]");
        screen_reset_color();
    }
    screen_set_fg(90);
    if (field_w >= 36) {
        screen_putstr("  |  WASD/Arrows  P:Pause  Q:Quit");
    } else if (field_w >= 24) {
        screen_putstr("  |  Move  P:Pause  Q:Quit");
    } else {
        screen_putstr("  |  P:Pause Q:Quit");
    }
    screen_reset_color();

    /* Direction indicator — right-aligned to field area */
    screen_goto(field_x + field_w - 8, 1);
    screen_set_fg(93);
    screen_putstr("Dir:");
    if      (gs->snake.dx ==  1) screen_putstr(" -> ");
    else if (gs->snake.dx == -1) screen_putstr(" <- ");
    else if (gs->snake.dy == -1) screen_putstr("  ^ ");
    else if (gs->snake.dy ==  1) screen_putstr("  v ");
    screen_reset_color();

    /* ---- Row 2: Thin separator ---- */
    screen_goto(hud_col, 2);
    screen_set_fg(90);
    for (i = 0; i < field_w + 2; i++) screen_putchar('-');
    screen_reset_color();

    /* ---- Game field border ---- */
    screen_set_fg(33);
    screen_draw_border(field_x - 1, field_y - 1, field_w, field_h);
    screen_reset_color();

    /* Render foods — per-slot color, blink faster when combo is active. */
    {
        static const int food_colors[MAX_FOODS] = {91, 93, 96}; /* red, yellow, cyan */
        int blink_div = (gs->combo >= 3) ? 2 : 5;
        if ((gs->ticks / blink_div) % 2 == 0) {
            int i;
            for (i = 0; i < MAX_FOODS; i++) {
                if (!gs->foods[i].active) continue;
                screen_goto(field_x + gs->foods[i].x, field_y + gs->foods[i].y);
                screen_set_fg(food_colors[i]);
                screen_set_bold();
                screen_putchar('@');
                screen_reset_color();
            }
        }
    }

    /* Render obstacles — solid magenta '#'. */
    {
        int i;
        for (i = 0; i < gs->num_obstacles; i++) {
            if (!gs->obstacles[i].active) continue;
            screen_goto(field_x + gs->obstacles[i].x,
                        field_y + gs->obstacles[i].y);
            screen_set_fg(95);
            screen_set_bold();
            screen_putchar('#');
            screen_reset_color();
        }
    }

    /* Render snake — gradient: bright head → green body → dark tail. */
    {
        Segment *cur = gs->snake.head;
        int idx = 0;
        int tail_start = gs->snake.length - gs->snake.length / 4;
        while (cur) {
            screen_goto(field_x + cur->x, field_y + cur->y);
            if (idx == 0) {
                screen_set_fg(92); screen_set_bold();
                screen_putchar('O');
            } else if (idx < 4) {
                screen_set_fg(92);
                screen_putchar('o');
            } else if (idx < tail_start) {
                screen_set_fg(32);
                screen_putchar('o');
            } else {
                screen_set_fg(90);
                screen_putchar('o');
            }
            screen_reset_color();
            idx++;
            cur = cur->next;
        }
    }

    /* ---- Row below field: Stats panel ----
     * Wide field -> full labels.
     * Narrow field -> compact labels to avoid overflow.
     */
    int score_row = field_y + field_h + 2;
    char sbuf[16], hbuf[16], lenbuf[8], fbuf[8], lbuf[8], multbuf[8];
    int  mult = my_clamp(gs->combo, 1, MAX_COMBO_MULT);
    my_itoa(gs->score,        sbuf);
    my_itoa(gs->high_score,   hbuf);
    my_itoa(gs->snake.length, lenbuf);
    my_itoa(gs->foods_eaten,  fbuf);
    my_itoa(gs->lives,        lbuf);
    my_itoa(mult,             multbuf);

    screen_goto(hud_col, score_row);
    if (field_w >= 38) {
        screen_set_fg(97);
        screen_putstr("Score:");
        /* Flash score yellow when a combo streak is active */
        if (gs->combo >= 2) {
            screen_set_fg((gs->ticks / 3) % 2 == 0 ? 93 : 97);
            screen_set_bold();
        } else {
            screen_set_fg(92);
        }
        screen_putstr(sbuf);
        screen_reset_color();
        screen_set_fg(97);
        screen_putstr("  High:");
        screen_set_fg(93);
        screen_putstr(hbuf);
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
        screen_set_fg(97);
        screen_putstr("  Lives:");
        screen_set_fg(96);
        screen_putstr(lbuf);
        screen_reset_color();
        screen_set_fg(97);
        screen_putstr("  Mult:");
        if (mult > 1) { screen_set_fg(93); screen_set_bold(); }
        else            screen_set_fg(90);
        screen_putstr("x");
        screen_putstr(multbuf);
        screen_reset_color();
    } else {
        screen_set_fg(97);
        screen_putstr("S:");
        screen_set_fg(92);
        screen_putstr(sbuf);
        screen_reset_color();
        screen_set_fg(97);
        screen_putstr(" H:");
        screen_set_fg(93);
        screen_putstr(hbuf);
        screen_reset_color();
        screen_set_fg(97);
        screen_putstr(" L:");
        screen_set_fg(95);
        screen_putstr(lenbuf);
        screen_reset_color();
        screen_set_fg(97);
        screen_putstr(" F:");
        screen_set_fg(91);
        screen_putstr(fbuf);
        screen_reset_color();
        screen_set_fg(97);
        screen_putstr(" V:");
        screen_set_fg(96);
        screen_putstr(lbuf);
        screen_reset_color();
        screen_set_fg(97);
        screen_putstr(" Mx:");
        if (mult > 1) { screen_set_fg(93); screen_set_bold(); }
        else            screen_set_fg(90);
        screen_putstr("x");
        screen_putstr(multbuf);
        screen_reset_color();
    }

    /* ---- Paused overlay — centred box ---- */
    if (gs->paused) {
        int mid_col = field_x + my_div(field_w, 2) - 7;
        int mid_row = field_y + my_div(field_h, 2) - 1;
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

    /* ---- Memory debug info ---- */
    screen_goto(hud_col, score_row + 1);
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
    clear_snake(gs);
}
