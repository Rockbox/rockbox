#include <string.h>
#include "plugin.h"
#include "inttypes.h"
#include <tlsf.h>

#ifndef MIKMOD_SUPP_H
#define MIKMOD_SUPP_H

#undef WIN32

#ifndef NO_MMSUPP_DEFINES
#define snprintf(...)		rb->snprintf(__VA_ARGS__)
#define fdprintf(...)		rb->fdprintf(__VA_ARGS__)
#define vsnprintf(...)		rb->vsnprintf(__VA_ARGS__)

#define srand(a)			rb->srand(a)
#define rand()				rb->rand()
#define qsort(a,b,c,d)		rb->qsort(a,b,c,d)
#define atoi(a)				rb->atoi(a)

#undef strlen
#define strlen(a)           rb->strlen(a)
#undef strcpy
#define strcpy(a,b)         rb->strcpy(a,b)
#undef strcat
#define strcat(a,b)         rb->strcat(a,b)
#undef strncmp
#define strncmp(a,b,c)      rb->strncmp(a,b,c)
#undef strcasecmp
#define strcasecmp(a,b)     rb->strcasecmp(a,b)

#undef open
#define open(a,b)		rb->open(a,b)
#undef lseek
#define lseek(a,b,c)	rb->lseek(a,b,c)
#undef close
#define close(a)		rb->close(a)
#undef read
#define read(a,b,c)		rb->read(a,b,c)
#undef write
#define write(a,b,c)	rb->write(a,b,c)
#undef filesize
#define filesize(a)		rb->filesize(a)
#endif

#define malloc(x)		tlsf_malloc(x)
#define free(x)			tlsf_free(x)
#define realloc(x,y)	tlsf_realloc(x,y)
#define calloc(x,y)		tlsf_calloc(x,y)

#undef strncat
#define strncat mmsupp_strncat
#undef printf
#define printf mmsupp_printf
#undef sprintf
#define sprintf mmsupp_sprintf

#define fprintf(...)

char* mmsupp_strncat(char *s1, const char *s2, size_t n);
void mmsupp_printf(const char *fmt, ...);
int mmsupp_sprintf(char *buf, const char *fmt, ... );

extern const struct plugin_api * rb;


#define SAMPLE_RATE SAMPR_44        /* 44100 */

#define BUF_SIZE 4096*8
#define NBUF   2

/* LibMikMod defines */
#define HAVE_SNPRINTF 1

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#endif
