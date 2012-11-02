#include "stddef.h"

void init_memory_pool(void *ptr, size_t size);
void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
