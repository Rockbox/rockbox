/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

ARMv6 vector math copyright (C) 2008 Jens Arnold

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110, USA

*/

#define FUSED_VECTOR_MATH

#if ORDER > 16
#define REPEAT_BLOCK(x) x x x
#else
#define REPEAT_BLOCK(x) x
#endif

/* Calculate scalarproduct, then add a 2nd vector (fused for performance)
 * This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned. It also requires that f2 and s2 are either both 32 bit
 * aligned or both unaligned. If either condition isn't met, it will either
 * result in a data abort or incorrect results. */
static inline int32_t vector_sp_add(int16_t* v1, int16_t* f2, int16_t* s2)
{
    int res;
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

    asm volatile (
#if ORDER > 32
        "mov     %[res], #0              \n"
#endif
        "tst     %[f2], #2               \n"
        "beq     20f                     \n"

    "10:                                 \n"
        "ldrh    r3, [%[f2]], #2         \n"
        "ldrh    r6, [%[s2]], #2         \n"
        "ldmia   %[f2]!, {r2,r4}         \n"
        "mov     r3, r3, lsl #16         \n"
        "mov     r6, r6, lsl #16         \n"

    "1:                                  \n"
        "ldmia   %[s2]!, {r5,r7}         \n"
        "pkhtb   r3, r3, r2              \n"
        "pkhtb   r2, r2, r4              \n"
        "ldrd    r0, [%[v1]]             \n"
        "mov     r5, r5, ror #16         \n"
        "pkhtb   r6, r5, r6, asr #16     \n"
        "pkhbt   r5, r5, r7, lsl #16     \n"
#if ORDER > 32
        "smladx  %[res], r0, r3, %[res]  \n"
#else
        "smuadx  %[res], r0, r3          \n"
#endif
        "smladx  %[res], r1, r2, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "sadd16  r0, r0, r6              \n"
        "sadd16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"

        REPEAT_BLOCK(
        "ldmia   %[s2]!, {r5,r6}         \n"
        "pkhtb   r4, r4, r2              \n"
        "pkhtb   r2, r2, r3              \n"
        "ldrd    r0, [%[v1]]             \n"
        "mov     r5, r5, ror #16         \n"
        "pkhtb   r7, r5, r7, asr #16     \n"
        "pkhbt   r5, r5, r6, lsl #16     \n"
        "smladx  %[res], r0, r4, %[res]  \n"
        "smladx  %[res], r1, r2, %[res]  \n"
        "ldmia   %[f2]!, {r2,r4}         \n"
        "sadd16  r0, r0, r7              \n"
        "sadd16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"
        "ldmia   %[s2]!, {r5,r7}         \n"
        "pkhtb   r3, r3, r2              \n"
        "pkhtb   r2, r2, r4              \n"
        "ldrd    r0, [%[v1]]             \n"
        "mov     r5, r5, ror #16         \n"
        "pkhtb   r6, r5, r6, asr #16     \n"
        "pkhbt   r5, r5, r7, lsl #16     \n"
        "smladx  %[res], r0, r3, %[res]  \n"
        "smladx  %[res], r1, r2, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "sadd16  r0, r0, r6              \n"
        "sadd16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"
        )

        "ldmia   %[s2]!, {r5,r6}         \n"
        "pkhtb   r4, r4, r2              \n"
        "pkhtb   r2, r2, r3              \n"
        "ldrd    r0, [%[v1]]             \n"
        "mov     r5, r5, ror #16         \n"
        "pkhtb   r7, r5, r7, asr #16     \n"
        "pkhbt   r5, r5, r6, lsl #16     \n"
        "smladx  %[res], r0, r4, %[res]  \n"
        "smladx  %[res], r1, r2, %[res]  \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "ldmneia %[f2]!, {r2,r4}         \n"
        "sadd16  r0, r0, r7              \n"
        "sadd16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"
        "bne     1b                      \n"
#else
        "sadd16  r0, r0, r7              \n"
        "sadd16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"
#endif

        "b       99f                     \n"

    "20:                                 \n"
        "ldrd    r4, [%[f2]], #8         \n"
        "ldrd    r0, [%[v1]]             \n"

#if ORDER > 32
    "1:                                  \n"
        "smlad   %[res], r0, r4, %[res]  \n"
#else
        "smuad   %[res], r0, r4          \n"
#endif
        "ldrd    r6, [%[s2]], #8         \n"
        "smlad   %[res], r1, r5, %[res]  \n"
        "ldrd    r4, [%[f2]], #8         \n"
        "ldrd    r2, [%[v1], #8]         \n"
        "sadd16  r0, r0, r6              \n"
        "sadd16  r1, r1, r7              \n"
        "strd    r0, [%[v1]], #8         \n"

        REPEAT_BLOCK(
        "smlad   %[res], r2, r4, %[res]  \n"
        "ldrd    r6, [%[s2]], #8         \n"
        "smlad   %[res], r3, r5, %[res]  \n"
        "ldrd    r4, [%[f2]], #8         \n"
        "ldrd    r0, [%[v1], #8]         \n"
        "sadd16  r2, r2, r6              \n"
        "sadd16  r3, r3, r7              \n"
        "strd    r2, [%[v1]], #8         \n"
        "smlad   %[res], r0, r4, %[res]  \n"
        "ldrd    r6, [%[s2]], #8         \n"
        "smlad   %[res], r1, r5, %[res]  \n"
        "ldrd    r4, [%[f2]], #8         \n"
        "ldrd    r2, [%[v1], #8]         \n"
        "sadd16  r0, r0, r6              \n"
        "sadd16  r1, r1, r7              \n"
        "strd    r0, [%[v1]], #8         \n"
        )

        "smlad   %[res], r2, r4, %[res]  \n"
        "ldrd    r6, [%[s2]], #8         \n"
        "smlad   %[res], r3, r5, %[res]  \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "ldrned  r4, [%[f2]], #8         \n"
        "ldrned  r0, [%[v1], #8]         \n"
        "sadd16  r2, r2, r6              \n"
        "sadd16  r3, r3, r7              \n"
        "strd    r2, [%[v1]], #8         \n"
        "bne     1b                      \n"
#else
        "sadd16  r2, r2, r6              \n"
        "sadd16  r3, r3, r7              \n"
        "strd    r2, [%[v1]], #8         \n"
#endif

    "99:                                 \n"
        : /* outputs */
#if ORDER > 32
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [f2] "+r"(f2),
        [s2] "+r"(s2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4",
        "r5", "r6", "r7", "cc", "memory"
    );
    return res;
}

/* Calculate scalarproduct, then subtract a 2nd vector (fused for performance)
 * This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned. It also requires that f2 and s2 are either both 32 bit
 * aligned or both unaligned. If either condition isn't met, it will either
 * result in a data abort or incorrect results. */
static inline int32_t vector_sp_sub(int16_t* v1, int16_t* f2, int16_t* s2)
{
    int res;
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

    asm volatile (
#if ORDER > 32
        "mov     %[res], #0              \n"
#endif
        "tst     %[f2], #2               \n"
        "beq     20f                     \n"

    "10:                                 \n"
        "ldrh    r3, [%[f2]], #2         \n"
        "ldrh    r6, [%[s2]], #2         \n"
        "ldmia   %[f2]!, {r2,r4}         \n"
        "mov     r3, r3, lsl #16         \n"
        "mov     r6, r6, lsl #16         \n"

    "1:                                  \n"
        "ldmia   %[s2]!, {r5,r7}         \n"
        "pkhtb   r3, r3, r2              \n"
        "pkhtb   r2, r2, r4              \n"
        "ldrd    r0, [%[v1]]             \n"
        "mov     r5, r5, ror #16         \n"
        "pkhtb   r6, r5, r6, asr #16     \n"
        "pkhbt   r5, r5, r7, lsl #16     \n"
#if ORDER > 32
        "smladx  %[res], r0, r3, %[res]  \n"
#else
        "smuadx  %[res], r0, r3          \n"
#endif
        "smladx  %[res], r1, r2, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "ssub16  r0, r0, r6              \n"
        "ssub16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"

        REPEAT_BLOCK(
        "ldmia   %[s2]!, {r5,r6}         \n"
        "pkhtb   r4, r4, r2              \n"
        "pkhtb   r2, r2, r3              \n"
        "ldrd    r0, [%[v1]]             \n"
        "mov     r5, r5, ror #16         \n"
        "pkhtb   r7, r5, r7, asr #16     \n"
        "pkhbt   r5, r5, r6, lsl #16     \n"
        "smladx  %[res], r0, r4, %[res]  \n"
        "smladx  %[res], r1, r2, %[res]  \n"
        "ldmia   %[f2]!, {r2,r4}         \n"
        "ssub16  r0, r0, r7              \n"
        "ssub16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"
        "ldmia   %[s2]!, {r5,r7}         \n"
        "pkhtb   r3, r3, r2              \n"
        "pkhtb   r2, r2, r4              \n"
        "ldrd    r0, [%[v1]]             \n"
        "mov     r5, r5, ror #16         \n"
        "pkhtb   r6, r5, r6, asr #16     \n"
        "pkhbt   r5, r5, r7, lsl #16     \n"
        "smladx  %[res], r0, r3, %[res]  \n"
        "smladx  %[res], r1, r2, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "ssub16  r0, r0, r6              \n"
        "ssub16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"
        )

        "ldmia   %[s2]!, {r5,r6}         \n"
        "pkhtb   r4, r4, r2              \n"
        "pkhtb   r2, r2, r3              \n"
        "ldrd    r0, [%[v1]]             \n"
        "mov     r5, r5, ror #16         \n"
        "pkhtb   r7, r5, r7, asr #16     \n"
        "pkhbt   r5, r5, r6, lsl #16     \n"
        "smladx  %[res], r0, r4, %[res]  \n"
        "smladx  %[res], r1, r2, %[res]  \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "ldmneia %[f2]!, {r2,r4}         \n"
        "ssub16  r0, r0, r7              \n"
        "ssub16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"
        "bne     1b                      \n"
#else
        "ssub16  r0, r0, r7              \n"
        "ssub16  r1, r1, r5              \n"
        "strd    r0, [%[v1]], #8         \n"
#endif

        "b       99f                     \n"

    "20:                                 \n"
        "ldrd    r4, [%[f2]], #8         \n"
        "ldrd    r0, [%[v1]]             \n"

#if ORDER > 32
    "1:                                  \n"
        "smlad   %[res], r0, r4, %[res]  \n"
#else
        "smuad   %[res], r0, r4          \n"
#endif
        "ldrd    r6, [%[s2]], #8         \n"
        "smlad   %[res], r1, r5, %[res]  \n"
        "ldrd    r4, [%[f2]], #8         \n"
        "ldrd    r2, [%[v1], #8]         \n"
        "ssub16  r0, r0, r6              \n"
        "ssub16  r1, r1, r7              \n"
        "strd    r0, [%[v1]], #8         \n"

        REPEAT_BLOCK(
        "smlad   %[res], r2, r4, %[res]  \n"
        "ldrd    r6, [%[s2]], #8         \n"
        "smlad   %[res], r3, r5, %[res]  \n"
        "ldrd    r4, [%[f2]], #8         \n"
        "ldrd    r0, [%[v1], #8]         \n"
        "ssub16  r2, r2, r6              \n"
        "ssub16  r3, r3, r7              \n"
        "strd    r2, [%[v1]], #8         \n"
        "smlad   %[res], r0, r4, %[res]  \n"
        "ldrd    r6, [%[s2]], #8         \n"
        "smlad   %[res], r1, r5, %[res]  \n"
        "ldrd    r4, [%[f2]], #8         \n"
        "ldrd    r2, [%[v1], #8]         \n"
        "ssub16  r0, r0, r6              \n"
        "ssub16  r1, r1, r7              \n"
        "strd    r0, [%[v1]], #8         \n"
        )

        "smlad   %[res], r2, r4, %[res]  \n"
        "ldrd    r6, [%[s2]], #8         \n"
        "smlad   %[res], r3, r5, %[res]  \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "ldrned  r4, [%[f2]], #8         \n"
        "ldrned  r0, [%[v1], #8]         \n"
        "ssub16  r2, r2, r6              \n"
        "ssub16  r3, r3, r7              \n"
        "strd    r2, [%[v1]], #8         \n"
        "bne     1b                      \n"
#else
        "ssub16  r2, r2, r6              \n"
        "ssub16  r3, r3, r7              \n"
        "strd    r2, [%[v1]], #8         \n"
#endif

    "99:                                 \n"
        : /* outputs */
#if ORDER > 32
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [f2] "+r"(f2),
        [s2] "+r"(s2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4",
        "r5", "r6", "r7", "cc", "memory"
    );
    return res;
}

/* This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned, otherwise it will result either in a data abort, or
 * incorrect results (if ARM aligncheck is disabled). */
static inline int32_t scalarproduct(int16_t* v1, int16_t* v2)
{
    int res;
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

    asm volatile (
#if ORDER > 32
        "mov     %[res], #0              \n"
#endif
        "tst     %[v2], #2               \n"
        "beq     20f                     \n"

    "10:                                 \n"
        "bic     %[v2], %[v2], #2        \n"
        "ldmia   %[v2]!, {r5-r7}         \n"
        "ldrd    r0, [%[v1]], #8         \n"

    "1:                                  \n"
        "pkhtb   r3, r5, r6              \n"
        "ldrd    r4, [%[v2]], #8         \n"
#if ORDER > 32
        "smladx  %[res], r0, r3, %[res]  \n"
#else
        "smuadx  %[res], r0, r3          \n"
#endif
        REPEAT_BLOCK(
        "pkhtb   r0, r6, r7              \n"
        "ldrd    r2, [%[v1]], #8         \n"
        "smladx  %[res], r1, r0, %[res]  \n"
        "pkhtb   r1, r7, r4              \n"
        "ldrd    r6, [%[v2]], #8         \n"
        "smladx  %[res], r2, r1, %[res]  \n"
        "pkhtb   r2, r4, r5              \n"
        "ldrd    r0, [%[v1]], #8         \n"
        "smladx  %[res], r3, r2, %[res]  \n"
        "pkhtb   r3, r5, r6              \n"
        "ldrd    r4, [%[v2]], #8         \n"
        "smladx  %[res], r0, r3, %[res]  \n"
        )

        "pkhtb   r0, r6, r7              \n"
        "ldrd    r2, [%[v1]], #8         \n"
        "smladx  %[res], r1, r0, %[res]  \n"
        "pkhtb   r1, r7, r4              \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "ldrned  r6, [%[v2]], #8         \n"
        "smladx  %[res], r2, r1, %[res]  \n"
        "pkhtb   r2, r4, r5              \n"
        "ldrned  r0, [%[v1]], #8         \n"
        "smladx  %[res], r3, r2, %[res]  \n"
        "bne     1b                      \n"
#else
        "pkhtb   r4, r4, r5              \n"
        "smladx  %[res], r2, r1, %[res]  \n"
        "smladx  %[res], r3, r4, %[res]  \n"
#endif

        "b       99f                     \n"

    "20:                                 \n"
        "ldrd    r0, [%[v1]], #8         \n"
        "ldmia   %[v2]!, {r5-r7}         \n"

    "1:                                  \n"
        "ldrd    r2, [%[v1]], #8         \n"
#if ORDER > 32
        "smlad   %[res], r0, r5, %[res]  \n"
#else
        "smuad   %[res], r0, r5          \n"
#endif
        REPEAT_BLOCK(
        "ldrd    r4, [%[v2]], #8         \n"
        "smlad   %[res], r1, r6, %[res]  \n"
        "ldrd    r0, [%[v1]], #8         \n"
        "smlad   %[res], r2, r7, %[res]  \n"
        "ldrd    r6, [%[v2]], #8         \n"
        "smlad   %[res], r3, r4, %[res]  \n"
        "ldrd    r2, [%[v1]], #8         \n"
        "smlad   %[res], r0, r5, %[res]  \n"
        )

#if ORDER > 32
        "ldrd    r4, [%[v2]], #8         \n"
        "smlad   %[res], r1, r6, %[res]  \n"
        "subs    %[cnt], %[cnt], #1      \n"
        "ldrned  r0, [%[v1]], #8         \n"
        "smlad   %[res], r2, r7, %[res]  \n"
        "ldrned  r6, [%[v2]], #8         \n"
        "smlad   %[res], r3, r4, %[res]  \n"
        "bne     1b                      \n"
#else
        "ldr     r4, [%[v2]], #4         \n"
        "smlad   %[res], r1, r6, %[res]  \n"
        "smlad   %[res], r2, r7, %[res]  \n"
        "smlad   %[res], r3, r4, %[res]  \n"
#endif

    "99:                                 \n"
        : /* outputs */
#if ORDER > 32
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [v2] "+r"(v2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3",
        "r4", "r5", "r6", "r7", "cc", "memory"
    );
    return res;
}
