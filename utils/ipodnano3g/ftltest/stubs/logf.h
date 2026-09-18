#include <stdio.h>
#define logf(...) (fprintf(stderr, "logf: " __VA_ARGS__), fputc('\n', stderr))
