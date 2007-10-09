/*
 * idct.c
 * Copyright (C) 2000-2003 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "plugin.h"

#include "mpeg2dec_config.h"

#include "mpeg2.h"
#include "attributes.h"
#include "mpeg2_internal.h"

/* 101100011001 */
#define W1 2841 /* 2048 * sqrt (2) * cos (1 * pi / 16) */
/* 101001110100 */
#define W2 2676 /* 2048 * sqrt (2) * cos (2 * pi / 16) */
/* 100101101000 */
#define W3 2408 /* 2048 * sqrt (2) * cos (3 * pi / 16) */
/* 011001001001 */
#define W5 1609 /* 2048 * sqrt (2) * cos (5 * pi / 16) */
/* 010001010100 */
#define W6 1108 /* 2048 * sqrt (2) * cos (6 * pi / 16) */
/* 001000110101 */
#define W7 565  /* 2048 * sqrt (2) * cos (7 * pi / 16) */

/* idct main entry point  */
void (* mpeg2_idct_copy) (int16_t * block, uint8_t * dest, int stride);
void (* mpeg2_idct_add) (int last, int16_t * block,
                         uint8_t * dest, int stride);

/*
 * In legal streams, the IDCT output should be between -384 and +384.
 * In corrupted streams, it is possible to force the IDCT output to go
 * to +-3826 - this is the worst case for a column IDCT where the
 * column inputs are 16-bit values.
 */
#if 0
#define BUTTERFLY(t0,t1,W0,W1,d0,d1) \
    do {                             \
        t0 = W0 * d0 + W1 * d1;      \
        t1 = W0 * d1 - W1 * d0;      \
    } while (0)
#else
#define BUTTERFLY(t0,t1,W0,W1,d0,d1) \
    do {                             \
        int tmp = W0 * (d0 + d1);    \
        t0 = tmp + (W1 - W0) * d1;   \
        t1 = tmp - (W1 + W0) * d0;   \
    } while (0)
#endif

/* Custom calling convention:
 * r0 contains block pointer and is non-volatile
 * all non-volatile c context saved and restored on its behalf
 */
static void idct (int16_t * block) __attribute__((naked,used));
static void idct (int16_t * block)
{
    asm volatile (
        "add    r12, r0, #128         \n"
    "1:                               \n"
        "ldrsh  r1, [r0, #0]          \n" /* d0 */
        "ldrsh  r2, [r0, #2]          \n" /* d1 */
        "ldrsh  r3, [r0, #4]          \n" /* d2 */
        "ldrsh  r4, [r0, #6]          \n" /* d3 */
        "ldrsh  r5, [r0, #8]          \n" /* d0 */
        "ldrsh  r6, [r0, #10]         \n" /* d1 */
        "ldrsh  r7, [r0, #12]         \n" /* d2 */
        "ldrsh  r8, [r0, #14]         \n" /* d3 */
        "orrs   r9, r2, r3            \n"
        "orreqs r9, r4, r5            \n"
        "orreqs r9, r6, r7            \n"
        "cmpeq  r8, #0                \n"
        "bne    2f                    \n"
        "mov    r1, r1, asl #15       \n"
        "bic    r1, r1, #0x8000       \n"
        "orr    r1, r1, r1, lsr #16   \n"
        "str    r1, [r0], #4          \n"
        "str    r1, [r0], #4          \n"
        "str    r1, [r0], #4          \n"
        "str    r1, [r0], #4          \n"
        "cmp    r0, r12               \n"
        "blo    1b                    \n"
        "b      3f                    \n"
    "2:                               \n"
        "mov    r1, r1, asl #11       \n" /* r1 = d0 = (block[0] << 11) + 2048 */
        "add    r1, r1, #2048         \n"
        "add    r1, r1, r3, asl #11   \n" /* r1 = t0 = d0 + (block[2] << 11) */
        "sub    r3, r1, r3, asl #12   \n" /* r3 = t1 = d0 - (block[2] << 11) */

        "add    r9, r2, r4            \n" /* r9 = tmp = (d1+d3)*(1108/4) */
        "add    r10, r9, r9, asl #2   \n"
        "add    r10, r10, r9, asl #4  \n"
        "add    r9, r10, r9, asl #8   \n"

        "add    r10, r2, r2, asl #4   \n" /* r2 = t2 = tmp + (d1*(1568/32)*8) */
        "add    r2, r10, r2, asl #5   \n"
        "add    r2, r9, r2, asl #3    \n"

        "add    r10, r4, r4, asl #2   \n" /* r4 = t3 = tmp - (d3*(3784/8)*2) */
        "rsb    r10, r10, r4, asl #6  \n"
        "add    r4, r4, r10, asl #3   \n"
        "sub    r4, r9, r4, asl #1    \n"
        /* t2 & t3 are 1/4 final value here */
        "add    r1, r1, r2, asl #2    \n" /* r1 = a0 = t0 + t2 */
        "sub    r2, r1, r2, asl #3    \n" /* r2 = a3 = t0 - t2 */
        "add    r3, r3, r4, asl #2    \n" /* r3 = a1 = t1 + t3 */
        "sub    r4, r3, r4, asl #3    \n" /* r4 = a2 = t1 - t3 */

        "add    r9, r8, r5            \n" /* r9 = tmp = 565*(d3 + d0) */
        "add    r10, r9, r9, asl #4   \n"
        "add    r10, r10, r10, asl #5 \n"
        "add    r9, r10, r9, asl #2   \n"

        "add    r10, r5, r5, asl #4   \n" /* r5 = t0 = tmp + (((2276/4)*d0)*4) */
        "add    r10, r10, r10, asl #5 \n"
        "add    r5, r10, r5, asl #3   \n"
        "add    r5, r9, r5, asl #2    \n"

        "add    r10, r8, r8, asl #2   \n" /* r8 = t1 = tmp - (((3406/2)*d3)*2) */
        "add    r10, r10, r10, asl #4 \n"
        "add    r10, r10, r8, asl #7  \n"
        "rsb    r8, r8, r10, asl #3   \n"
        "sub    r8, r9, r8, asl #1    \n"

        "add    r9, r6, r7            \n" /* r9 = tmp = (2408/8)*(d1 + d2) */
        "add    r10, r9, r9, asl #3   \n"
        "add    r10, r10, r10, asl #5 \n"
        "add    r9, r10, r9, asl #2   \n"

        "add    r10, r7, r7, asl #3   \n" /* r7 = t2 = (tmp*8) - 799*d2 */
        "add    r10, r10, r7, asl #4  \n"
        "rsb    r7, r7, r10, asl #5   \n"
        "rsb    r7, r7, r9, asl #3    \n"

        "sub    r10, r6, r6, asl #4   \n" /* r6 = t3 = (tmp*8) - 4017*d1 */
        "sub    r10, r10, r6, asl #6  \n"
        "add    r10, r10, r6, asl #12 \n"
        "add    r6, r10, r6           \n"
        "rsb    r6, r6, r9, asl #3    \n"
         /* t0 = r5, t1 = r8, t2 = r7, t3 = r6*/
        "add    r9, r5, r7            \n" /* r9 = b0 = t0 + t2 */
        "add    r10, r8, r6           \n" /* r10 = b3 = t1 + t3 */
        "sub    r5, r5, r7            \n" /* t0 -= t2 */
        "sub    r8, r8, r6            \n" /* t1 -= t3 */
        "add    r6, r5, r8            \n" /* r6 = t0 + t1 */
        "sub    r7, r5, r8            \n" /* r7 = t0 - t1 */

        "add    r11, r6, r6, asr #2   \n" /* r6 = b1 = r6*(181/128) */
        "add    r11, r11, r11, asr #5 \n"
        "add    r6, r11, r6, asr #3   \n"
        "add    r11, r7, r7, asr #2   \n" /* r7 = b2 = r7*(181/128) */
        "add    r11, r11, r11, asr #5 \n"
        "add    r7, r11, r7, asr #3   \n"
        /* r1 = a0, r3 = a1,   r4 = a2,   r2 = a3 */
        /* r9 = b0, r6 = b1*2, r7 = b2*2, r10 = b3 */
        "add    r5, r1, r9            \n" /* block[0] = (a0 + b0) >> 12 */
        "mov    r5, r5, asr #12       \n"
        "strh   r5, [r0], #2          \n"
        "add    r8, r3, r6, asr #1    \n" /* block[1] = (a1 + b1) >> 12 */
        "mov    r8, r8, asr #12       \n"
        "strh   r8, [r0], #2          \n"
        "add    r5, r4, r7, asr #1    \n" /* block[2] = (a2 + b2) >> 12 */
        "mov    r5, r5, asr #12       \n"
        "strh   r5, [r0], #2          \n"
        "add    r8, r2, r10           \n" /* block[3] = (a3 + b3) >> 12 */
        "mov    r8, r8, asr #12       \n"
        "strh   r8, [r0], #2          \n"
        "sub    r5, r2, r10           \n" /* block[4] = (a3 - b3) >> 12 */
        "mov    r5, r5, asr #12       \n"
        "strh   r5, [r0], #2          \n"
        "sub    r8, r4, r7, asr #1    \n" /* block[5] = (a2 - b2) >> 12 */
        "mov    r8, r8, asr #12       \n"
        "strh   r8, [r0], #2          \n"
        "sub    r5, r3, r6, asr #1    \n" /* block[6] = (a1 - b1) >> 12 */
        "mov    r5, r5, asr #12       \n"
        "strh   r5, [r0], #2          \n"
        "sub    r8, r1, r9            \n" /* block[7] = (a0 - b0) >> 12 */
        "mov    r8, r8, asr #12       \n"
        "strh   r8, [r0], #2          \n"
        "cmp    r0, r12               \n"
        "blo    1b                    \n"
    "3:                               \n"
        "sub    r0, r0, #128          \n"
        "add    r12, r0, #16          \n"
    "4:                               \n"
        "ldrsh  r1, [r0, #0*8]        \n" /* d0 */
        "ldrsh  r2, [r0, #2*8]        \n" /* d1 */
        "ldrsh  r3, [r0, #4*8]        \n" /* d2 */
        "ldrsh  r4, [r0, #6*8]        \n" /* d3 */
        "ldrsh  r5, [r0, #8*8]        \n" /* d0 */
        "ldrsh  r6, [r0, #10*8]       \n" /* d1 */
        "ldrsh  r7, [r0, #12*8]       \n" /* d2 */
        "ldrsh  r8, [r0, #14*8]       \n" /* d3 */

        "mov    r1, r1, asl #11       \n" /* r1 = d0 = (block[0] << 11) + 2048 */
        "add    r1, r1, #65536        \n"
        "add    r1, r1, r3, asl #11   \n" /* r1 = t0 = d0 + d2:(block[2] << 11) */
        "sub    r3, r1, r3, asl #12   \n" /* r3 = t1 = d0 - d2:(block[2] << 11) */

        "add    r9, r2, r4            \n" /* r9 = tmp = (d1+d3)*(1108/4) */
        "add    r10, r9, r9, asl #2   \n"
        "add    r10, r10, r9, asl #4  \n"
        "add    r9, r10, r9, asl #8   \n"

        "add    r11, r2, r2, asl #4   \n" /* r2 = t2 = tmp + (d1*(1568/32)*8) */
        "add    r2, r11, r2, asl #5   \n"
        "add    r2, r9, r2, asl #3    \n"

        "add    r10, r4, r4, asl #2   \n" /* r4 = t3 = tmp - (d3*(3784/8)*2) */
        "rsb    r10, r10, r4, asl #6  \n"
        "add    r4, r4, r10, asl #3   \n"
        "sub    r4, r9, r4, asl #1    \n"
        /* t2 & t3 are 1/4 final value here */
        "add    r1, r1, r2, asl #2    \n" /* r1 = a0 = t0 + t2 */
        "sub    r2, r1, r2, asl #3    \n" /* r2 = a3 = t0 - t2 */
        "add    r3, r3, r4, asl #2    \n" /* r3 = a1 = t1 + t3 */
        "sub    r4, r3, r4, asl #3    \n" /* r4 = a2 = t1 - t3 */

        "add    r9, r8, r5            \n" /* r9 = tmp = 565*(d3 + d0) */
        "add    r10, r9, r9, asl #4   \n"
        "add    r10, r10, r10, asl #5 \n"
        "add    r9, r10, r9, asl #2   \n"

        "add    r10, r5, r5, asl #4   \n" /* r5 = t0 = tmp + (((2276/4)*d0)*4) */
        "add    r10, r10, r10, asl #5 \n"
        "add    r5, r10, r5, asl #3   \n"
        "add    r5, r9, r5, asl #2    \n"

        "add    r10, r8, r8, asl #2   \n" /* r8 = t1 = tmp - (((3406/2)*d3)*2) */
        "add    r10, r10, r10, asl #4 \n"
        "add    r10, r10, r8, asl #7  \n"
        "rsb    r8, r8, r10, asl #3   \n"
        "sub    r8, r9, r8, asl #1    \n"

        "add    r9, r6, r7            \n" /* r9 = tmp = (2408/8)*(d1 + d2) */
        "add    r10, r9, r9, asl #3   \n"
        "add    r10, r10, r10, asl #5 \n"
        "add    r9, r10, r9, asl #2   \n"

        "add    r10, r7, r7, asl #3   \n" /* r7 = t2 = (tmp*8) - 799*d2 */
        "add    r10, r10, r7, asl #4  \n"
        "rsb    r7, r7, r10, asl #5   \n"
        "rsb    r7, r7, r9, asl #3    \n"

        "sub    r10, r6, r6, asl #4   \n" /* r6 = t3 = (tmp*8) - 4017*d1 */
        "sub    r10, r10, r6, asl #6  \n"
        "add    r10, r10, r6, asl #12 \n"
        "add    r6, r10, r6           \n"
        "rsb    r6, r6, r9, asl #3    \n"
                                         /* t0=r5, t1=r8, t2=r7, t3=r6*/
        "add    r9, r5, r7            \n" /* r9 = b0 = t0 + t2 */
        "add    r10, r8, r6           \n" /* r10 = b3 = t1 + t3 */
        "sub    r5, r5, r7            \n" /* t0 -= t2 */
        "sub    r8, r8, r6            \n" /* t1 -= t3 */
        "add    r6, r5, r8            \n" /* r6 = t0 + t1 */
        "sub    r7, r5, r8            \n" /* r7 = t0 - t1 */

        "add    r11, r6, r6, asr #2   \n" /* r6 = b1 = r5*(181/128) */
        "add    r11, r11, r11, asr #5 \n"
        "add    r6, r11, r6, asr #3   \n"
        "add    r11, r7, r7, asr #2   \n" /* r7 = b2 = r6*(181/128) */
        "add    r11, r11, r11, asr #5 \n"
        "add    r7, r11, r7, asr #3   \n"
        /* r1 = a0, r3 = a1,   r4 = a2,    r2 = a3 */
        /* r9 = b0, r6 = b1*2, r7 = b2*2, r10 = b3 */
        "add    r5, r1, r9            \n" /* block[0] = (a0 + b0) >> 17 */
        "mov    r5, r5, asr #17       \n"
        "strh   r5, [r0, #0*8]        \n"
        "add    r8, r3, r6, asr #1    \n" /* block[1] = (a1 + b1) >> 17 */
        "mov    r8, r8, asr #17       \n"
        "strh   r8, [r0, #2*8]        \n"
        "add    r5, r4, r7, asr #1    \n" /* block[2] = (a2 + b2) >> 17 */
        "mov    r5, r5, asr #17       \n"
        "strh   r5, [r0, #4*8]        \n"
        "add    r8, r2, r10           \n" /* block[3] = (a3 + b3) >> 17 */
        "mov    r8, r8, asr #17       \n"
        "strh   r8, [r0, #6*8]        \n"
        "sub    r5, r2, r10           \n" /* block[4] = (a3 - b3) >> 17 */
        "mov    r5, r5, asr #17       \n"
        "strh   r5, [r0, #8*8]        \n"
        "sub    r8, r4, r7, asr #1    \n" /* block[5] = (a2 - b2) >> 17 */
        "mov    r8, r8, asr #17       \n"
        "strh   r8, [r0, #10*8]       \n"
        "sub    r5, r3, r6, asr #1    \n" /* block[6] = (a1 - b1) >> 17 */
        "mov    r5, r5, asr #17       \n"
        "strh   r5, [r0, #12*8]       \n"
        "sub    r8, r1, r9            \n" /* block[7] = (a0 - b0) >> 17 */
        "mov    r8, r8, asr #17       \n"
        "strh   r8, [r0, #14*8]       \n"
        "add    r0, r0, #2            \n"
        "cmp    r0, r12               \n"
        "blo    4b                    \n"
        "sub    r0, r0, #16           \n"
        "bx     lr                    \n"
    );
    (void)block;
}

static void mpeg2_idct_copy_c (int16_t * block, uint8_t * dest,
                               const int stride) __attribute__((naked));
static void mpeg2_idct_copy_c (int16_t * block, uint8_t * dest,
                               const int stride)
{
    asm volatile(
        "stmfd  sp!, { r1-r2, \
                       r4-r12, lr }   \n"
        "bl     idct                  \n"
        "ldmfd  sp!, { r1-r2 }        \n"
        "mov    r11, #0               \n"
        "add    r12, r0, #128         \n"
    "1:                               \n"
        "ldrsh  r3, [r0, #0]          \n"
        "ldrsh  r4, [r0, #2]          \n"
        "ldrsh  r5, [r0, #4]          \n"
        "ldrsh  r6, [r0, #6]          \n"
        "ldrsh  r7, [r0, #8]          \n"
        "ldrsh  r8, [r0, #10]         \n"
        "ldrsh  r9, [r0, #12]         \n"
        "ldrsh  r10, [r0, #14]        \n"
        "cmp    r3, #255              \n"
        "mvnhi  r3, r3, asr #31       \n"
        "strb   r3, [r1, #0]          \n"
        "str    r11, [r0], #4         \n"
        "cmp    r4, #255              \n"
        "mvnhi  r4, r4, asr #31       \n"
        "strb   r4, [r1, #1]          \n"
        "cmp    r5, #255              \n"
        "mvnhi  r5, r5, asr #31       \n"
        "strb   r5, [r1, #2]          \n"
        "str    r11, [r0], #4         \n"
        "cmp    r6, #255              \n"
        "mvnhi  r6, r6, asr #31       \n"
        "strb   r6, [r1, #3]          \n"
        "cmp    r7, #255              \n"
        "mvnhi  r7, r7, asr #31       \n"
        "strb   r7, [r1, #4]          \n"
        "str    r11, [r0], #4         \n"
        "cmp    r8, #255              \n"
        "mvnhi  r8, r8, asr #31       \n"
        "strb   r8, [r1, #5]          \n"
        "cmp    r9, #255              \n"
        "mvnhi  r9, r9, asr #31       \n"
        "strb   r9, [r1, #6]          \n"
        "str    r11, [r0], #4         \n"
        "cmp    r10, #255             \n"
        "mvnhi  r10, r10, asr #31     \n"
        "strb   r10, [r1, #7]         \n"
        "add    r1, r1, r2            \n"
        "cmp    r0, r12               \n"
        "blo    1b                    \n"
        "ldmfd  sp!, { r4-r12, pc }   \n"
    );
    (void)block; (void)dest; (void)stride;
}

static void mpeg2_idct_add_c (int last, int16_t * block,
                              uint8_t * dest, const int stride) __attribute__((naked));
static void mpeg2_idct_add_c (int last, int16_t * block,
                              uint8_t * dest, const int stride)
{
    asm volatile (
        "cmp    r0, #129              \n"
        "mov    r0, r1                \n"
        "ldreqsh r1, [r0, #0]         \n"
        "bne    1f                    \n"
        "and    r1, r1, #0x70         \n"
        "cmp    r1, #0x40             \n"
        "bne    3f                    \n"
    "1:                               \n"
        "stmfd  sp!, { r2-r12, lr }   \n"
        "bl     idct                  \n"
        "ldmfd  sp!, { r1-r2 }        \n"
        "mov    r11, #0               \n"
        "add    r12, r0, #128         \n"
    "2:                               \n"
        "ldrb   r3, [r1, #0]          \n"
        "ldrb   r4, [r1, #1]          \n"
        "ldrb   r5, [r1, #2]          \n"
        "ldrb   r6, [r1, #3]          \n"
        "ldrsh  r7, [r0, #0]          \n"
        "ldrsh  r8, [r0, #2]          \n"
        "ldrsh  r9, [r0, #4]          \n"
        "ldrsh  r10, [r0, #6]         \n"
        "add    r7, r7, r3            \n"
        "ldrb   r3, [r1, #4]          \n"
        "cmp    r7, #255              \n"
        "mvnhi  r7, r7, asr #31       \n"
        "strb   r7, [r1, #0]          \n"
        "ldrsh  r7, [r0, #8]          \n"
        "add    r8, r8, r4            \n"
        "ldrb   r4, [r1, #5]          \n"
        "cmp    r8, #255              \n"
        "mvnhi  r8, r8, asr #31       \n"
        "strb   r8, [r1, #1]          \n"
        "ldrsh  r8, [r0, #10]         \n"
        "add    r9, r9, r5            \n"
        "ldrb   r5, [r1, #6]          \n"
        "cmp    r9, #255              \n"
        "mvnhi  r9, r9, asr #31       \n"
        "strb   r9, [r1, #2]          \n"
        "ldrsh  r9, [r0, #12]         \n"
        "add    r10, r10, r6          \n"
        "ldrb   r6, [r1, #7]          \n"
        "cmp    r10, #255             \n"
        "mvnhi  r10, r10, asr #31     \n"
        "strb   r10, [r1, #3]         \n"
        "ldrsh  r10, [r0, #14]        \n"
        "str    r11, [r0], #4         \n"
        "add    r7, r7, r3            \n"
        "cmp    r7, #255              \n"
        "mvnhi  r7, r7, asr #31       \n"
        "strb   r7, [r1, #4]          \n"
        "str    r11, [r0], #4         \n"
        "add    r8, r8, r4            \n"
        "cmp    r8, #255              \n"
        "mvnhi  r8, r8, asr #31       \n"
        "strb   r8, [r1, #5]          \n"
        "str    r11, [r0], #4         \n"
        "add    r9, r9, r5            \n"
        "cmp    r9, #255              \n"
        "mvnhi  r9, r9, asr #31       \n"
        "strb   r9, [r1, #6]          \n"
        "add    r10, r10, r6          \n"
        "cmp    r10, #255             \n"
        "mvnhi  r10, r10, asr #31     \n"
        "strb   r10, [r1, #7]         \n"
        "str    r11, [r0], #4         \n"
        "add    r1, r1, r2            \n"
        "cmp    r0, r12               \n"
        "blo    2b                    \n"
        "ldmfd  sp!, { r4-r12, pc }   \n"
    "3:                               \n"
        "stmfd  sp!, { r4-r11 }       \n"
        "ldrsh  r1, [r0, #0]          \n" /* r1 = block[0] */
        "mov    r11, #0               \n"
        "strh   r11, [r0, #0]         \n" /* block[0] = 0 */
        "strh   r11, [r0, #126]       \n" /* block[63] = 0 */
        "add    r1, r1, #64           \n" /* r1 = DC << 7 */
        "add    r0, r2, r3, asl #3    \n"
    "4:                               \n"
        "ldrb   r4, [r2, #0]          \n"
        "ldrb   r5, [r2, #1]          \n"
        "ldrb   r6, [r2, #2]          \n"
        "ldrb   r7, [r2, #3]          \n"
        "ldrb   r8, [r2, #4]          \n"
        "ldrb   r9, [r2, #5]          \n"
        "ldrb   r10, [r2, #6]         \n"
        "ldrb   r11, [r2, #7]         \n"
        "add    r4, r4, r1, asr #7    \n"
        "cmp    r4, #255              \n"
        "mvnhi  r4, r4, asr #31       \n"
        "strb   r4, [r2, #0]          \n"
        "add    r5, r5, r1, asr #7    \n"
        "cmp    r5, #255              \n"
        "mvnhi  r5, r5, asr #31       \n"
        "strb   r5, [r2, #1]          \n"
        "add    r6, r6, r1, asr #7    \n"
        "cmp    r6, #255              \n"
        "mvnhi  r6, r6, asr #31       \n"
        "strb   r6, [r2, #2]          \n"
        "add    r7, r7, r1, asr #7    \n"
        "cmp    r7, #255              \n"
        "mvnhi  r7, r7, asr #31       \n"
        "strb   r7, [r2, #3]          \n"
        "add    r8, r8, r1, asr #7    \n"
        "cmp    r8, #255              \n"
        "mvnhi  r8, r8, asr #31       \n"
        "strb   r8, [r2, #4]          \n"
        "add    r9, r9, r1, asr #7    \n"
        "cmp    r9, #255              \n"
        "mvnhi  r9, r9, asr #31       \n"
        "strb   r9, [r2, #5]          \n"
        "add    r10, r10, r1, asr #7  \n"
        "cmp    r10, #255             \n"
        "mvnhi  r10, r10, asr #31     \n"
        "strb   r10, [r2, #6]         \n"
        "add    r11, r11, r1, asr #7  \n"
        "cmp    r11, #255             \n"
        "mvnhi  r11, r11, asr #31     \n"
        "strb   r11, [r2, #7]         \n"
        "add    r2, r2, r3            \n"
        "cmp    r2, r0                \n"
        "blo    4b                    \n"
        "ldmfd  sp!, { r4-r11 }       \n"
        "bx     lr                    \n"
    );
    (void)last; (void)block; (void)dest; (void)stride;
}

void mpeg2_idct_init (void)
{
    extern uint8_t default_mpeg2_scan_norm[64];
    extern uint8_t default_mpeg2_scan_alt[64];
    extern uint8_t mpeg2_scan_norm[64];
    extern uint8_t mpeg2_scan_alt[64];
    int i, j;

    mpeg2_idct_copy = mpeg2_idct_copy_c;
    mpeg2_idct_add = mpeg2_idct_add_c;

    for (i = 0; i < 64; i++)
    {
        j = default_mpeg2_scan_norm[i];
        mpeg2_scan_norm[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);

        j = default_mpeg2_scan_alt[i];
        mpeg2_scan_alt[i] = ((j & 0x36) >> 1) | ((j & 0x09) << 2);
    }
}
