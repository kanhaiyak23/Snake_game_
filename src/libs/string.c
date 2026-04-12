/*
 * string.c — Custom String Library Implementation
 * No <string.h> used anywhere. Everything is manual byte-by-byte work.
 */
#include "string.h"
#include "math.h"

/* ---------------------------------------------------------------
 * Length: count bytes until null terminator.
 * --------------------------------------------------------------- */
int my_strlen(const char *s) {
    int len = 0;
    while (s[len] != '\0') len++;
    return len;
}

/* ---------------------------------------------------------------
 * Copy src into dst (including null terminator).
 * --------------------------------------------------------------- */
void my_strcpy(char *dst, const char *src) {
    int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
}

/* ---------------------------------------------------------------
 * Compare two strings. Returns 0 if equal, negative if a<b,
 * positive if a>b.
 * --------------------------------------------------------------- */
int my_strcmp(const char *a, const char *b) {
    int i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
        i++;
    }
    return (int)a[i] - (int)b[i];
}

int my_strncmp(const char *a, const char *b, int n) {
    int i = 0;
    while (i < n && a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
        i++;
    }
    if (i == n) return 0;
    return (int)a[i] - (int)b[i];
}

/* ---------------------------------------------------------------
 * Append src onto end of dst (dst must have enough space).
 * --------------------------------------------------------------- */
void my_strcat(char *dst, const char *src) {
    int di = my_strlen(dst);
    int si = 0;
    while (src[si] != '\0') dst[di++] = src[si++];
    dst[di] = '\0';
}

/* ---------------------------------------------------------------
 * Integer to decimal string.
 * Handles 0, positive, and negative numbers.
 * --------------------------------------------------------------- */
void my_itoa(int n, char *buf) {
    int i = 0;
    int neg = 0;

    if (n == 0) { buf[0] = '0'; buf[1] = '\0'; return; }

    if (n < 0) { neg = 1; n = my_abs(n); }

    /* Build digits in reverse */
    char tmp[20];
    int t = 0;
    while (n > 0) {
        tmp[t++] = (char)('0' + my_mod(n, 10));
        n = my_div(n, 10);
    }
    if (neg) buf[i++] = '-';
    /* Reverse tmp into buf */
    int j;
    for (j = t - 1; j >= 0; j--) buf[i++] = tmp[j];
    buf[i] = '\0';
}

/* ---------------------------------------------------------------
 * Simple tokenizer: scans the string for the next token delimited
 * by `delim`. Call with the original string on first call; pass
 * NULL on subsequent calls (like strtok).
 * --------------------------------------------------------------- */
static char *strtok_ptr = (void*)0;

char *my_strtok(char *s, char delim) {
    if (s != (void*)0) strtok_ptr = s;
    if (strtok_ptr == (void*)0 || *strtok_ptr == '\0') return (void*)0;

    /* Skip leading delimiters */
    while (*strtok_ptr == delim) strtok_ptr++;
    if (*strtok_ptr == '\0') return (void*)0;

    char *start = strtok_ptr;
    while (*strtok_ptr != '\0' && *strtok_ptr != delim) strtok_ptr++;
    if (*strtok_ptr == delim) { *strtok_ptr = '\0'; strtok_ptr++; }

    return start;
}
