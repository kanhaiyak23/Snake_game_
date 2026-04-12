#ifndef STRING_H_CUSTOM
#define STRING_H_CUSTOM

/* string.h — Custom String Library
 * No <string.h> is used. All string operations are implemented from scratch.
 */

int  my_strlen(const char *s);
void my_strcpy(char *dst, const char *src);
int  my_strcmp(const char *a, const char *b);
void my_itoa(int n, char *buf);          /* integer → decimal string      */
char *my_strtok(char *s, char delim);    /* simple single-char tokenizer   */
void my_strcat(char *dst, const char *src);
int  my_strncmp(const char *a, const char *b, int n);

#endif /* STRING_H_CUSTOM */
