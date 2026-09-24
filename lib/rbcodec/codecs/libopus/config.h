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

/* Code in IRAM.  PP5022 and PP5024 give a codec an 80 KB IRAM window and Opus
   already spends 39 KB of it on tables and .ibss; the hot decode path is
   about 34 KB of code, so it fits with room over.  Every other PP has a 48 KB
   window, which the tables would leave too little of, and the AS3525 codec
   link defines no IRAM region at all because the whole codec already runs
   from IRAM there.  libcook splits the same way, for the same reason.
   Measured on a Sansa e200v1: 48.96 MHz to 42.92, a 12.3% cut that the cycle
   model cannot see at all, because it does not model an instruction cache.
   Define OPUS_NO_ICODE to build without it and compare. */
#if (CONFIG_CPU == PP5022 || CONFIG_CPU == PP5024) && !defined(OPUS_NO_ICODE)
#define OPUS_ARM_ICODE            /* also read by the .S kernels */
#endif

#ifndef __ASSEMBLER__
#ifdef OPUS_ARM_ICODE
#define ICODE_ATTR_OPUS   ICODE_ATTR
#else
#define ICODE_ATTR_OPUS
#endif
#endif

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
/* Upstream's OPUS_ARM_INLINE_ASM and OPUS_ARM_INLINE_EDSP, renamed: upstream
   layers EDSP on top of ASM, but here exactly one is defined per core. */
#if ARM_ARCH == 4
#define OPUS_ARM_ASM_ARMV4_ONLY
#elif ARM_ARCH > 4
#define OPUS_ARM_ASM_ARMV5E_AND_LATER
#if (ARCH_PROFILE == ARM_PROFILE_MICRO)
#define OPUS_ARM_NO_FFT_ASM
#define OPUS_ARM_NO_MDCT_ASM
#define OPUS_ARM_NO_PITCH_ASM
#define OPUS_NO_PFA
#define OPUS_ARM_NO_COMB_ASM
#define OPUS_ARM_NO_BANDS_ASM
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

/* Mixed-radix leftovers: built, but not worth IRAM once the prime
   factor transform makes them unreachable. */
#ifdef OPUS_PFA
#define ICODE_ATTR_OPUS_MR
#define ICONST_ATTR_OPUS_MR
#else
#define ICODE_ATTR_OPUS_MR  ICODE_ATTR_OPUS
#define ICONST_ATTR_OPUS_MR ICONST_ATTR
#endif

#if defined(CPU_COLDFIRE)
#define OPUS_CF_INLINE_ASM
#endif

#endif /* CONFIG_H */
