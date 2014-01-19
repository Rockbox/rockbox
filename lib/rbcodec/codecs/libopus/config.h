#ifndef CONFIG_H
#define CONFIG_H

#include "rbcodecconfig.h"
#include "codeclib.h"
#include "ogg/ogg.h"

/* general stuff */
#define OPUS_BUILD

/* alloc stuff */
#define VAR_ARRAYS
#define NORM_ALIASING_HACK

#define OVERRIDE_OPUS_ALLOC
#define OVERRIDE_OPUS_FREE
#define OVERRIDE_OPUS_ALLOC_SCRATCH

#define opus_alloc          _ogg_malloc
#define opus_free           _ogg_free
#define opus_alloc_scratch  _ogg_malloc

/* lrint */
#define HAVE_LRINTF 0
#define HAVE_LRINT  0

/* embedded stuff */
#define FIXED_POINT
#define DISABLE_FLOAT_API
#define EMBEDDED_ARM 1

/* undefinitions */
#ifdef ABS
#undef ABS
#endif
#ifdef MIN
#undef MIN
#endif
#ifdef MAX
#undef MAX
#endif

#if defined(CPU_ARM)
#define OPUS_ARM_ASM
#if ARM_ARCH == 4
#define OPUS_ARM_INLINE_ASM
#elif ARM_ARCH > 4
#define OPUS_ARM_INLINE_EDSP
#endif
#endif

#if defined(CPU_COLDFIRE)
#define OPUS_CF_INLINE_ASM
#endif

#endif /* CONFIG_H */

