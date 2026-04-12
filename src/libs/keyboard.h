#ifndef KEYBOARD_H
#define KEYBOARD_H

/* keyboard.h — Custom Keyboard Input Library
 * Provides non-blocking keypress detection and line reading.
 * Uses POSIX termios for raw mode (hardware abstraction — allowed).
 */

void keyboard_init(void);    /* put terminal in raw non-blocking mode */
void keyboard_restore(void); /* restore terminal to normal mode        */
char keyPressed(void);       /* non-blocking: returns char (>0) or 0   */
void readLine(char *buf, int max_len); /* blocking full-line read       */

/* Key code constants */
#define KEY_UP    'w'
#define KEY_DOWN  's'
#define KEY_LEFT  'a'
#define KEY_RIGHT 'd'
#define KEY_QUIT  'q'
#define KEY_PAUSE 'p'
#define KEY_ENTER '\n'

#endif /* KEYBOARD_H */
