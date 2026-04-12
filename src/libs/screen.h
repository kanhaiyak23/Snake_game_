#ifndef SCREEN_H
#define SCREEN_H

/* screen.h — Custom Terminal Screen Library
 * Uses ANSI escape codes with <stdio.h> I/O only.
 * Simulates a raster display on the terminal.
 */

void screen_init(void);
void screen_cleanup(void);    /* restore terminal before exit */
void screen_clear(void);
void screen_goto(int col, int row);     /* 1-indexed, col=x, row=y */
void screen_putchar(char c);
void screen_putstr(const char *s);
void screen_hide_cursor(void);
void screen_show_cursor(void);
void screen_draw_border(int x, int y, int w, int h);
void screen_flush(void);

/* Color and style helpers — ANSI SGR codes */
void screen_set_fg(int code);   /* foreground: 32=green, 31=red, 33=yellow, 90-97=bright */
void screen_set_bg(int code);   /* background: 42=green, 41=red, 40=black               */
void screen_set_bold(void);     /* bold text                                             */
void screen_reset_color(void);  /* reset all attributes                                 */

/* Screen coordinates for the game field */
#define FIELD_X      2       /* left border column (1-indexed)          */
#define FIELD_Y      3       /* top border row — row 1 reserved for title */
#define FIELD_W      40      /* inner width  in characters               */
#define FIELD_H      20      /* inner height in character rows           */

#endif /* SCREEN_H */
