#ifndef __MALLOC_H__
#define __MALLOC_H__

#include <stdlib.h>

#define malloc do_malloc
#define free do_free
#define realloc do_realloc

void *do_malloc(size_t size);
void do_free(void *ptr);
void *do_realloc(void *ptr, size_t size);

void malloc_stats();

#endif
