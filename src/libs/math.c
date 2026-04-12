/*
 * math.c — Custom Math Library Implementation
 * All arithmetic is built from first principles. No <math.h> used.
 */
#include "math.h"

/* ---------------------------------------------------------------
 * Multiplication via repeated addition
 * Handles negative operands.
 * --------------------------------------------------------------- */
int my_mul(int a, int b) {
    if (a == 0 || b == 0) return 0;
    int neg = 0;
    if (a < 0) { a = my_abs(a); neg = !neg; }
    if (b < 0) { b = my_abs(b); neg = !neg; }
    int result = 0;
    int i;
    for (i = 0; i < b; i++) result += a;
    return neg ? -result : result;
}

/* ---------------------------------------------------------------
 * Integer division via repeated subtraction
 * Handles negative operands. Returns 0 on divide-by-zero.
 * --------------------------------------------------------------- */
int my_div(int a, int b) {
    if (b == 0) return 0;
    int neg = 0;
    if (a < 0) { a = my_abs(a); neg = !neg; }
    if (b < 0) { b = my_abs(b); neg = !neg; }
    int result = 0;
    while (a >= b) { a -= b; result++; }
    return neg ? -result : result;
}

/* ---------------------------------------------------------------
 * Modulo via repeated subtraction
 * --------------------------------------------------------------- */
int my_mod(int a, int b) {
    if (b == 0) return 0;
    int neg = (a < 0);
    if (a < 0) a = my_abs(a);
    if (b < 0) b = my_abs(b);
    while (a >= b) a -= b;
    return neg ? -a : a;
}

/* ---------------------------------------------------------------
 * Absolute value without branching tricks — simple and clear.
 * --------------------------------------------------------------- */
int my_abs(int x) {
    return (x < 0) ? -x : x;
}

/* ---------------------------------------------------------------
 * Clamp a value between min and max (inclusive).
 * Used by screen / boundary checks.
 * --------------------------------------------------------------- */
int my_clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/* ---------------------------------------------------------------
 * Returns 1 if (x,y) is inside [0,w) x [0,h), else 0.
 * Used for wall and boundary collision detection.
 * --------------------------------------------------------------- */
int my_in_bounds(int x, int y, int w, int h) {
    if (x < 0 || x >= w) return 0;
    if (y < 0 || y >= h) return 0;
    return 1;
}

int my_max(int a, int b) { return (a > b) ? a : b; }
int my_min(int a, int b) { return (a < b) ? a : b; }

/* ---------------------------------------------------------------
 * XorShift32 pseudo-random number generator.
 * Uses only bit-shifts and XOR — O(1), no multiplication needed.
 * This is a well-known, fast, full-period PRNG with period 2^32-1.
 *
 * Why not LCG with my_mul?
 *   my_mul uses repeated addition (b iterations). With a seed from
 *   time(), b ≈ 1,700,000,000 → billions of iterations per call.
 *   XorShift32 is three XOR-shift ops regardless of seed value.
 * --------------------------------------------------------------- */
static unsigned int rng_state = 12345u;

void my_srand(unsigned int seed) {
    /* XorShift must never have state = 0 (it would lock at 0 forever) */
    rng_state = (seed == 0u) ? 2463534242u : seed;
}

int my_rand(void) {
    /* XorShift32 — Marsaglia (2003), period = 2^32 - 1 */
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (int)(rng_state & 0x7fffffff);
}

int my_rand_range(int lo, int hi) {
    if (hi <= lo) return lo;
    int range = hi - lo;
    return lo + my_mod(my_abs(my_rand()), range);
}
