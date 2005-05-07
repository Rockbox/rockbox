#include "../codec.h"

/* a52dec profiling */
/* #undef A52DEC_GPROF */

/* Define to 1 if you have the `memalign' function. */
/* #undef HAVE_MEMALIGN 1 */

/* liba52 djbfft support */
/* #undef LIBA52_DJBFFT */

/* a52 sample precision */
/* #undef LIBA52_DOUBLE */

/* use fixed-point arithmetic */
#define LIBA52_FIXED

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */

/* Used in bitstream.h */

#ifdef ROCKBOX_BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#endif

