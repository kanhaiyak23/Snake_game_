#ifndef SNAKE_H
#define SNAKE_H

/* snake.h — Snake Game Logic
 * All game data (segments, food) is managed via custom memory.c alloc/dealloc.
 */

/* ---- Data structures ---- */

typedef struct Segment {
    int x, y;              /* position on game field (0-indexed) */
    struct Segment *next;  /* linked list toward tail             */
} Segment;

typedef struct Snake {
    Segment *head;
    Segment *tail;
    int length;
    int dx, dy;            /* direction: one of {0,1,-1}          */
} Snake;

typedef struct Food {
    int x, y;
    int active;
} Food;

/* Why the session ended — used for game-over message (e.g. heap full). */
#define GAME_END_NONE       0
#define GAME_END_COLLISION  1  /* wall or self */
#define GAME_END_OOM        2  /* alloc() failed — pool exhausted / fragmentation */
#define GAME_END_QUIT       3  /* user pressed Q */
#define GAME_END_INTERNAL   4  /* broken invariant; should never run in normal play */

typedef struct GameState {
    Snake   snake;
    Food    food;
    int     score;
    int     high_score;
    int     level;
    int     game_over;
    int     paused;
    int     ticks;         /* frame counter                       */
    int     foods_eaten;   /* total food items consumed this game */
    int     game_end_reason; /* GAME_END_* when game_over is set */
    int     lives;         /* remaining lives (starts at 3)       */
    int     just_died;     /* set to 1 for one cycle after losing a life */
} GameState;

/* ---- API ---- */

void game_init(GameState *gs, int prev_high_score);
void game_update(GameState *gs);            /* advance one tick           */
void game_handle_input(GameState *gs, char key);
void game_render(const GameState *gs);      /* draw everything            */
void game_free(GameState *gs);              /* dealloc all segments       */
int  game_speed_delay(const GameState *gs); /* microseconds between ticks */

#endif /* SNAKE_H */
