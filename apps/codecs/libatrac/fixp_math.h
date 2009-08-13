#include <stdlib.h>
#ifdef ROCKBOX
#include "asm_arm.h"
#include "asm_mcf5249.h"
#include "codeclib_misc.h"
#endif

/* Macros for converting between various fixed-point representations and floating point. */
#define ONE_16 (1L << 16)
#define fixtof64(x)       (float)((float)(x) / (float)(1 << 16))        //does not work on int64_t!
#define ftofix32(x)       ((int32_t)((x) * (float)(1 << 16) + ((x) < 0 ? -0.5 : 0.5)))
#define ftofix31(x)       ((int32_t)((x) * (float)(1 << 31) + ((x) < 0 ? -0.5 : 0.5)))
#define fix31tof64(x)     (float)((float)(x) / (float)(1 << 31))

/* Fixed point math routines for use in atrac3.c */
#ifdef ROCKBOX
#define fixmul31(x,y) (MULT31(x,y))
#define fixmul16(x,y) (MULT32(x,y))
#else
inline int32_t fixmul16(int32_t x, int32_t y);
inline int32_t fixmul31(int32_t x, int32_t y);
#endif /* ROCKBOX */
inline int32_t fixdiv16(int32_t x, int32_t y);
inline int32_t fastSqrt(int32_t n);
