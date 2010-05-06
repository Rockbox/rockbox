#ifndef HELPERS_H
#define HELPERS_H

extern int my_getc(int);
extern int my_putc(char,int);
extern off_t my_ftell(int);
extern void *my_malloc(size_t size);


#undef getc
#define getc my_getc
#undef putc
#define putc my_putc
#undef ftell
#define ftell my_ftell
#define malloc my_malloc

#endif /* HELPERS_H */
