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

/* Rockbox decodes Opus and never encodes it: neither celt_encoder.c nor
   opus_encoder.c is listed in SOURCES.  Saying so lets gcc fold away the
   encoder halves of the routines celt/bands.c shares between the two
   directions, which is worth 2,976 bytes of code and, because those branches
   were holding values live across quant_partition's recursive calls, a
   measurable amount of its stack spill.  celt_encoder.c carries an #error
   against this define, so adding an encoder back fails the build loudly
   rather than miscompiling. */
#define CELT_DECODE_ONLY

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
#if (ARCH_PROFILE == ARM_PROFILE_MICRO)
#define OPUS_ARM_NO_FFT_ASM
#define OPUS_ARM_NO_MDCT_ASM
#define OPUS_ARM_NO_PITCH_ASM
#define OPUS_NO_PFA
#endif
#endif /* ARM_ARCH */

/*optimization no hardware division support*/
#if !defined(ARM_HAVE_HW_DIV)
#define OPUS_EC_DECODE_DIV
#endif

/* Exact lookup table for bitexact_log2tan.  Saves about 0.25 MHz if the
   tables can live in IRAM, which only the ARMv4 PP5022/PP5024 have room for,
   and nothing otherwise, so it is only built there. */
#if CONFIG_CPU == PP5022 || CONFIG_CPU == PP5024
#define OPUS_LOG2TAN_TABLE
#endif
#endif

/* Good-Thomas FFT for the backward MDCT.  Every 48 kHz CELT transform length
   is 15 times a power of two, so the prime factor algorithm applies and the
   inter-stage twiddles -- 73% of the FFT multiplies -- disappear.  Define
   OPUS_NO_PFA to fall back to the mixed-radix chain. */
#ifndef OPUS_NO_PFA
#define OPUS_PFA
#endif

#if defined(CPU_COLDFIRE)
#define OPUS_CF_INLINE_ASM
#endif

#endif /* CONFIG_H */
