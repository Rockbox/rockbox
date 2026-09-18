#ifndef CONFIG_H
#define CONFIG_H

/* rbcodecconfig.h pulls in the firmware config.h, which is assembly-safe and
   is what the kernels under celt/arm come here for.  Everything below it is
   C, so the .S files skip it. */
#include "rbcodecconfig.h"
#ifndef __ASSEMBLER__
#include "codeclib.h"
#include "ogg/ogg.h"
#endif

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

/*optimization no hardware division support*/
#if !defined(ARM_HAVE_HW_DIV)
#define OPUS_EC_DECODE_DIV
#endif

#endif

#if defined(CPU_COLDFIRE)
#define OPUS_CF_INLINE_ASM
#endif

#endif /* CONFIG_H */

