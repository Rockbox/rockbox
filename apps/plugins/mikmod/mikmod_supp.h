#include <string.h>
#include "plugin.h"
#include "inttypes.h"
#include <tlsf.h>

#ifndef MIKMOD_SUPP_H
#define MIKMOD_SUPP_H

#undef WIN32

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


#ifdef SIMULATOR 
#define SAMPLE_RATE SAMPR_44		/* Simulator requires 44100Hz */
#else
#if (HW_SAMPR_CAPS & SAMPR_CAP_22)  /* use 22050Hz if we can */
#define SAMPLE_RATE SAMPR_22        /* 22050 */
#else
#define SAMPLE_RATE SAMPR_44        /* 44100 */
#endif
#endif

#define BUF_SIZE 4096*8
#define NBUF   2

/* LibMikMod defines */
#define HAVE_SNPRINTF 1

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#endif
