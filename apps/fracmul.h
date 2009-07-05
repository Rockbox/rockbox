#ifndef _FRACMUL_H
#define _FRACMUL_H

/** FRACTIONAL MULTIPLICATION - TAKEN FROM apps/dsp.h
 *  Multiply two fixed point numbers with 31 fractional bits:
 *      FRACMUL(x, y)
 *
 *  Multiply two fixed point numbers with 31 fractional bits,
 *          then shift left by z bits:
 *      FRACMUL_SHL(x, y, z)
 *          NOTE: z must be in the range 1-8 on Coldfire targets.
 */


/* A bunch of fixed point assembler helper macros */
#if defined(CPU_COLDFIRE)
/* These macros use the Coldfire EMAC extension and need the MACSR flags set
 * to fractional mode with no rounding.
 */

/* Multiply two S.31 fractional integers and return the sign bit and the
 * 31 most significant bits of the result.
 */
#define FRACMUL(x, y) \
({ \
    long t; \
    asm ("mac.l    %[a], %[b], %%acc0\n\t" \
         "movclr.l %%acc0, %[t]\n\t" \
         : [t] "=r" (t) : [a] "r" (x), [b] "r" (y)); \
    t; \
})

/* Multiply two S.31 fractional integers, and return the 32 most significant
 * bits after a shift left by the constant z. NOTE: Only works for shifts of
 * 1 to 8 on Coldfire!
 */
#define FRACMUL_SHL(x, y, z) \
({ \
    long t, t2; \
    asm ("mac.l    %[a], %[b], %%acc0\n\t" \
         "moveq.l  %[d], %[t]\n\t" \
         "move.l   %%accext01, %[t2]\n\t" \
         "and.l    %[mask], %[t2]\n\t" \
         "lsr.l    %[t], %[t2]\n\t" \
         "movclr.l %%acc0, %[t]\n\t" \
         "asl.l    %[c], %[t]\n\t" \
         "or.l     %[t2], %[t]\n\t" \
         : [t] "=&d" (t), [t2] "=&d" (t2) \
         : [a] "r" (x), [b] "r" (y), [mask] "d" (0xff), \
           [c] "i" ((z)), [d] "i" (8 - (z))); \
    t; \
})

#elif defined(CPU_ARM)

/* Multiply two S.31 fractional integers and return the sign bit and the
 * 31 most significant bits of the result.
 */
#define FRACMUL(x, y) \
({ \
    long t, t2; \
    asm ("smull    %[t], %[t2], %[a], %[b]\n\t" \
         "mov      %[t2], %[t2], asl #1\n\t" \
         "orr      %[t], %[t2], %[t], lsr #31\n\t" \
         : [t] "=&r" (t), [t2] "=&r" (t2) \
         : [a] "r" (x), [b] "r" (y)); \
    t; \
})

/* Multiply two S.31 fractional integers, and return the 32 most significant
 * bits after a shift left by the constant z.
 */
#define FRACMUL_SHL(x, y, z) \
({ \
    long t, t2; \
    asm ("smull    %[t], %[t2], %[a], %[b]\n\t" \
         "mov      %[t2], %[t2], asl %[c]\n\t" \
         "orr      %[t], %[t2], %[t], lsr %[d]\n\t" \
         : [t] "=&r" (t), [t2] "=&r" (t2) \
         : [a] "r" (x), [b] "r" (y), \
           [c] "M" ((z) + 1), [d] "M" (31 - (z))); \
    t; \
})

#else

#define FRACMUL(x, y) (long) (((((long long) (x)) * ((long long) (y))) >> 31))
#define FRACMUL_SHL(x, y, z) \
((long)(((((long long) (x)) * ((long long) (y))) >> (31 - (z)))))

#endif

#endif
