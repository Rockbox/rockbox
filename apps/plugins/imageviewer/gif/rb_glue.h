#include "plugin.h"
#include <tlsf.h>

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

#define malloc(a) tlsf_malloc((a))
#define free(a)   tlsf_free((a))
#define realloc(a, b) tlsf_realloc((a),(b))
#define calloc(a,b) tlsf_calloc((a),(b))

#ifndef SIZE_MAX
#define SIZE_MAX INT_MAX
#endif
