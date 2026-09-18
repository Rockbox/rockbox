#include <stdio.h>
#include <stdlib.h>
#define panicf(...) (fprintf(stderr, "panicf: " __VA_ARGS__), \
                     fputc('\n', stderr), abort())
