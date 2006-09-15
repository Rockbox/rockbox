#ifndef HELPERS_H
#define HELPERS_H

extern int my_getc(int);
extern int my_putc(char,int);
extern off_t my_ftell(int);
extern void *my_malloc(size_t size);


#define getc my_getc
#define malloc my_malloc
#define ftell my_ftell
#define putc my_putc

#endif /* HELPERS_H */
