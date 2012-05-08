#ifndef PLATFORM_H_INCLUDED
#define PLATFORM_H_INCLUDED

#include "rbcodecconfig.h"
#include "rbcodecplatform.h"

/*

#ifndef ROCKBOX
# define __PCTOOL__
# define RBCODEC_NOT_ROCKBOX
# define ROCKBOX
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t siz);
#endif
*/
#ifndef ARRAYLEN
# define ARRAYLEN(a) (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef MIN
# define MIN(x, y) ((x)<(y) ? (x) : (y))
#endif

#ifndef MAX
# define MAX(x, y) ((x)>(y) ? (x) : (y))
#endif

#ifndef BIT_N
# define BIT_N(n) (1U << (n))
#endif
/*
#ifdef CODEC

# ifdef debugf
#  undef debugf
# endif

# ifdef logf
#  undef logf
# endif

#else

# ifndef DEBUGF
#  define DEBUGF debugf
# endif

# ifndef debugf
#  define debugf(...) do { } while (0)
# endif

# ifndef logf
#  define logf(...) do { } while (0)
# endif

#endif

#ifndef ATTRIBUTE_PRINTF
# define ATTRIBUTE_PRINTF(fmt, arg1)
#endif

#ifndef LIKELY
# define LIKELY(x) (x)
#endif

#ifndef UNLIKELY
# define UNLIKELY(x) (x)
#endif
*/
#ifndef CACHEALIGN_ATTR
# define CACHEALIGN_ATTR
#endif
/*
#ifndef DATA_ATTR
# define DATA_ATTR
#endif
*/
#ifndef IBSS_ATTR
# define IBSS_ATTR
#endif

#ifndef ICODE_ATTR
# define ICODE_ATTR
#endif

#ifndef ICONST_ATTR
# define ICONST_ATTR
#endif

#ifndef IDATA_ATTR
# define IDATA_ATTR
#endif
/*
#ifndef INIT_ATTR
# define INIT_ATTR
#endif
*/
#ifndef MEM_ALIGN_ATTR
# define MEM_ALIGN_ATTR
#endif

#ifndef CACHEALIGN_SIZE
# define CACHEALIGN_SIZE 1
#endif
/*
#ifndef HAVE_CLIP_SAMPLE_16
static inline int32_t clip_sample_16(int32_t sample)
{
    if ((int16_t)sample != sample)
        sample = 0x7fff ^ (sample >> 31);
    return sample;
}
#endif
*/
#endif /* PLATFORM_H_INCLUDED */
