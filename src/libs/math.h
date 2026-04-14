#ifndef MATH_H
#define MATH_H

/* math.h — Custom Math Library
 * No <math.h> is used. All arithmetic is implemented from scratch.
 */

int  my_mul(int a, int b);
int  my_div(int a, int b);
int  my_mod(int a, int b);
int  my_abs(int x);
int  my_clamp(int val, int min, int max);
int  my_in_bounds(int x, int y, int w, int h);
int  my_max(int a, int b);
int  my_min(int a, int b);

/* XorShift32 pseudo-random number generator (O(1) per call) */
void my_srand(unsigned int seed);
int  my_rand(void);
int  my_rand_range(int lo, int hi);  /* lo inclusive, hi exclusive */

#endif /* MATH_H */
