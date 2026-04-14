/*
 * math.c — Custom Math Library Implementation
 * All arithmetic is built from first principles. No <math.h> used.
 *
 * Complexity (32-bit int operands; bit-length k <= 32):
 *   multiply_ll — O(k) iterations (shift-and-add / Russian peasant)
 *   my_mul      — O(k) after swapping so the loop runs on min(|a|,|b|))
 *   divide_positive_ll — O(log(a/b)) recursion depth, O(1) per frame
 *   my_div      — same as positive kernel + O(1) sign fix
 *   my_mod      — O(my_div + my_mul)
 */
#include "math.h"

/* ---------------------------------------------------------------
 * multiply_ll — shift-and-add on non-negative magnitudes.
 * Invariant: computes la * lb for la >= 0, lb >= 0.
 * Time: O(number of bits in lb) <= O(32). Space: O(1).
 * --------------------------------------------------------------- */
static long long multiply_ll(long long la, long long lb) {
    long long result = 0;

    while (lb > 0) {
        if (lb & 1) {
            result += la;
        }
        la <<= 1;
        lb >>= 1;
    }
    return result;
}

/* ---------------------------------------------------------------
 * my_mul — product using shift-and-add; fewer iterations by swapping
 * so the right-shifted operand is min(|a|,|b|).
 * Time: O(log min(|a|,|b|)) bit steps. Space: O(1).
 * --------------------------------------------------------------- */
int my_mul(int a, int b) {
    int neg = 0;
    long long la = (long long)a;
    long long lb = (long long)b;

    if (la == 0 || lb == 0) return 0;
    if (la < 0) {
        la = -la;
        neg = !neg;
    }
    if (lb < 0) {
        lb = -lb;
        neg = !neg;
    }
    /* Fewer loop iterations: run bits of the smaller magnitude */
    if (la < lb) {
        long long t = la;
        la = lb;
        lb = t;
    }
    {
        long long r = multiply_ll(la, lb);
        if (neg) r = -r;
        return (int)r;
    }
}

/* ---------------------------------------------------------------
 * divide_positive_ll — floor(a/b) for 0 <= a, 0 < b.
 * Recursive doubling of divisor (same idea as binary long division).
 * Time: O(log(a/b)) depth. Space: O(log(a/b)) stack frames.
 * --------------------------------------------------------------- */
static long long divide_positive_ll(long long a, long long b) {
    if (b == 0) return 0;
    if (a < b) return 0;        //divsor greater than dividend then quotient is 0

    {
        long long q = divide_positive_ll(a, b << 1); //recursive call to divide_positive_ll with divisor doubled
        long long rem = a - ((q << 1) * b);   //remainder = dividend - (quotient * divisor)

        if (rem < b) {       //remainder less than divisor then quotient is 2q
            return q << 1;      //2q
        }
        return (q << 1) | 1;      //2q+1    //remainder greater than divisor then quotient is 2q+1
    }
}

/* ---------------------------------------------------------------
 * my_div — truncates toward zero.
 * Time: O(log(|a|/|b|)) for magnitudes + O(1) sign.
 * --------------------------------------------------------------- */
int my_div(int a, int b) {
    if (b == 0) return 0;

    {
        int neg = 0;
        long long la = (long long)a;
        long long lb = (long long)b;

        if (la < 0) {
            la = -la;
            neg = !neg;
        }
        if (lb < 0) {
            lb = -lb;
            neg = !neg;
        }
        {
            long long q = divide_positive_ll(la, lb);
            if (neg) q = -q;
            return (int)q;
        }
    }
}

/* ---------------------------------------------------------------
 * my_mod — consistent with truncating division:
 *   a == my_div(a,b)*b + my_mod(a,b)
 * Time: O(my_div + my_mul).
 * --------------------------------------------------------------- */
int my_mod(int a, int b) {
    if (b == 0) return 0;
    return a - my_mul(my_div(a, b), b);
}

/* ---------------------------------------------------------------
 * my_abs — Time: O(1). Space: O(1).
 * --------------------------------------------------------------- */
int my_abs(int x) {
    return (x < 0) ? -x : x;
}

/* ---------------------------------------------------------------
 * my_clamp — Time: O(1). Space: O(1).
 * --------------------------------------------------------------- */
int my_clamp(int val, int min, int max) {
    if (val < min) return min;
    if (val > max) return max;
    return val;
}

/* ---------------------------------------------------------------
 * my_in_bounds — Time: O(1). Space: O(1).
 * --------------------------------------------------------------- */
int my_in_bounds(int x, int y, int w, int h) {
    if (x < 0 || x >= w) return 0;
    if (y < 0 || y >= h) return 0;
    return 1;
}

/* ---------------------------------------------------------------
 * my_max / my_min — Time: O(1). Space: O(1).
 * --------------------------------------------------------------- */
int my_max(int a, int b) { return (a > b) ? a : b; }
int my_min(int a, int b) { return (a < b) ? a : b; }

/* ---------------------------------------------------------------
 * XorShift32 PRNG — O(1) per call; no multiply in hot path.
 * --------------------------------------------------------------- */
static unsigned int rng_state = 12345u;

void my_srand(unsigned int seed) {
    rng_state = (seed == 0u) ? 2463534242u : seed;
}

int my_rand(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return (int)(rng_state & 0x7fffffff);
}

/* ---------------------------------------------------------------
 * my_rand_range — O(1) RNG + my_mod cost.
 * --------------------------------------------------------------- */
int my_rand_range(int lo, int hi) {
    if (hi <= lo) return lo;
    int range = hi - lo;
    return lo + my_mod(my_abs(my_rand()), range);
}
