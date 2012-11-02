#include "plugin.h"
#include "dumb_alloc.h"

#undef  memset
#define memset(a,b,c)      rb->memset((a),(b),(c))
#undef  memmove
#define memmove(a,b,c)     rb->memmove((a),(b),(c))
#undef  memcmp
#define memcmp(a,b,c)      rb->memcmp((a),(b),(c))
#undef  strncmp
#define strncmp(a,b,c)     rb->strncmp((a),(b),(c))

#define fread(ptr, size, nmemb, stream) rb->read(stream, ptr, size*nmemb)
#define fclose(stream) rb->close(stream)
#define fdopen(a,b) ((a))

#ifndef SIZE_MAX
#define SIZE_MAX INT_MAX
#endif
