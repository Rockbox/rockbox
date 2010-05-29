/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

ARMv5te vector math copyright (C) 2008 Jens Arnold

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

#define REPEAT_3(x) x x x
#if ORDER > 16
#define REPEAT_MLA(x) x x x x x x x
#else
#define REPEAT_MLA(x) x x x
#endif

/* Calculate scalarproduct, then add a 2nd vector (fused for performance)
 * This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned. It also requires that f2 and s2 are either both 32 bit
 * aligned or both unaligned. If either condition isn't met, it will either
 * result in a data abort or incorrect results. */
static inline int32_t vector_sp_add(int16_t* v1, int16_t* f2, int16_t* s2)
{
    int res;
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

#define ADDHALFREGS(sum, s1, s2)                         /* Adds register   */ \
        "mov   " #s1  ", " #s1  ",   ror #16         \n" /* halves straight */ \
        "add   " #sum ", " #s1  ", " #s2  ", lsl #16 \n" /* Clobbers 's1'   */ \
        "add   " #s1  ", " #s1  ", " #s2  ", lsr #16 \n" \
        "mov   " #s1  ", " #s1  ",   lsl #16         \n" \
        "orr   " #sum ", " #s1  ", " #sum ", lsr #16 \n"

#define ADDHALFXREGS(sum, s1, s2)                        /* Adds register  */ \
        "add   " #s1  ", " #s1  ", " #sum ", lsl #16 \n" /* halves across. */ \
        "add   " #sum ", " #s2  ", " #sum ", lsr #16 \n" /* Clobbers 's1'. */ \
        "mov   " #sum ", " #sum ",   lsl #16         \n" \
        "orr   " #sum ", " #sum ", " #s1  ", lsr #16 \n"

    asm volatile (
#if ORDER > 16
        "mov     %[res], #0              \n"
#endif
        "tst     %[f2], #2               \n"
        "beq     20f                     \n"

    "10:                                 \n"
        "ldrh    r4, [%[s2]], #2         \n"
        "mov     r4, r4, lsl #16         \n"
        "ldrh    r3, [%[f2]], #2         \n"
#if ORDER > 16
        "mov     r3, r3, lsl #16         \n"
    "1:                                  \n"
        "ldmia   %[v1],  {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
#else
        "ldmia   %[v1],  {r0,r1}         \n"
        "smulbb  %[res], r0, r3          \n"
#endif
        "ldmia   %[f2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        "ldmia   %[s2]!, {r2,r5}         \n"
        ADDHALFXREGS(r0, r4, r2)
        ADDHALFXREGS(r1, r2, r5)
        "stmia   %[v1]!, {r0,r1}         \n"
        "ldmia   %[v1],  {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        "ldmia   %[s2]!, {r2,r4}         \n"
        ADDHALFXREGS(r0, r5, r2)
        ADDHALFXREGS(r1, r2, r4)
        "stmia   %[v1]!, {r0,r1}         \n"

        "ldmia   %[v1],  {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        "ldmia   %[s2]!, {r2,r5}         \n"
        ADDHALFXREGS(r0, r4, r2)
        ADDHALFXREGS(r1, r2, r5)
        "stmia   %[v1]!, {r0,r1}         \n"
        "ldmia   %[v1],  {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        "ldmia   %[s2]!, {r2,r4}         \n"
        ADDHALFXREGS(r0, r5, r2)
        ADDHALFXREGS(r1, r2, r4)
        "stmia   %[v1]!, {r0,r1}         \n"
#if ORDER > 16
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"
#endif
        "b       99f                     \n"

    "20:                                 \n"
    "1:                                  \n"
        "ldmia   %[v1],  {r1,r2}         \n"
        "ldmia   %[f2]!, {r3,r4}         \n"
#if ORDER > 16
        "smlabb  %[res], r1, r3, %[res]  \n"
#else
        "smulbb  %[res], r1, r3          \n"
#endif
        "smlatt  %[res], r1, r3, %[res]  \n"
        "smlabb  %[res], r2, r4, %[res]  \n"
        "smlatt  %[res], r2, r4, %[res]  \n"
        "ldmia   %[s2]!, {r3,r4}         \n"
        ADDHALFREGS(r0, r1, r3)
        ADDHALFREGS(r1, r2, r4)
        "stmia   %[v1]!, {r0,r1}         \n"

        REPEAT_3(
        "ldmia   %[v1],  {r1,r2}         \n"
        "ldmia   %[f2]!, {r3,r4}         \n"
        "smlabb  %[res], r1, r3, %[res]  \n"
        "smlatt  %[res], r1, r3, %[res]  \n"
        "smlabb  %[res], r2, r4, %[res]  \n"
        "smlatt  %[res], r2, r4, %[res]  \n"
        "ldmia   %[s2]!, {r3,r4}         \n"
        ADDHALFREGS(r0, r1, r3)
        ADDHALFREGS(r1, r2, r4)
        "stmia   %[v1]!, {r0,r1}         \n"
        )
#if ORDER > 16
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"
#endif

    "99:                                 \n"
        : /* outputs */
#if ORDER > 16
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [f2] "+r"(f2),
        [s2] "+r"(s2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "cc", "memory"
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
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

#define SUBHALFREGS(dif, s1, s2)                         /* Subtracts reg.  */ \
        "mov   " #s1  ", " #s1  ",   ror #16         \n" /* halves straight */ \
        "sub   " #dif ", " #s1  ", " #s2  ", lsl #16 \n" /* Clobbers 's1'   */ \
        "sub   " #s1  ", " #s1  ", " #s2  ", lsr #16 \n" \
        "mov   " #s1  ", " #s1  ",   lsl #16         \n" \
        "orr   " #dif ", " #s1  ", " #dif ", lsr #16 \n"

#define SUBHALFXREGS(dif, s1, s2, msk)                   /* Subtracts reg. */  \
        "sub   " #s1  ", " #dif ", " #s1  ", lsr #16 \n" /* halves across. */  \
        "and   " #s1  ", " #s1  ", " #msk "          \n" /* Needs msk =    */  \
        "rsb   " #dif ", " #s2  ", " #dif ", lsr #16 \n" /*    0x0000ffff, */  \
        "orr   " #dif ", " #s1  ", " #dif ", lsl #16 \n" /* clobbers 's1'. */

    asm volatile (
#if ORDER > 16
        "mov     %[res], #0              \n"
#endif
        "tst     %[f2], #2               \n"
        "beq     20f                     \n"

    "10:                                 \n"
        "mov     r6, #0xff               \n"
        "orr     r6, r6, #0xff00         \n"
        "ldrh    r4, [%[s2]], #2         \n"
        "mov     r4, r4, lsl #16         \n"
        "ldrh    r3, [%[f2]], #2         \n"
#if ORDER > 16
        "mov     r3, r3, lsl #16         \n"
    "1:                                  \n"
        "ldmia   %[v1],  {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
#else
        "ldmia   %[v1],  {r0,r1}         \n"
        "smulbb  %[res], r0, r3          \n"
#endif
        "ldmia   %[f2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        "ldmia   %[s2]!, {r2,r5}         \n"
        SUBHALFXREGS(r0, r4, r2, r6)
        SUBHALFXREGS(r1, r2, r5, r6)
        "stmia   %[v1]!, {r0,r1}         \n"
        "ldmia   %[v1],  {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        "ldmia   %[s2]!, {r2,r4}         \n"
        SUBHALFXREGS(r0, r5, r2, r6)
        SUBHALFXREGS(r1, r2, r4, r6)
        "stmia   %[v1]!, {r0,r1}         \n"

        "ldmia   %[v1],  {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        "ldmia   %[s2]!, {r2,r5}         \n"
        SUBHALFXREGS(r0, r4, r2, r6)
        SUBHALFXREGS(r1, r2, r5, r6)
        "stmia   %[v1]!, {r0,r1}         \n"
        "ldmia   %[v1],  {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
        "ldmia   %[f2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        "ldmia   %[s2]!, {r2,r4}         \n"
        SUBHALFXREGS(r0, r5, r2, r6)
        SUBHALFXREGS(r1, r2, r4, r6)
        "stmia   %[v1]!, {r0,r1}         \n"
#if ORDER > 16
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"
#endif
        "b       99f                     \n"

    "20:                                 \n"
    "1:                                  \n"
        "ldmia   %[v1],  {r1,r2}         \n"
        "ldmia   %[f2]!, {r3,r4}         \n"
#if ORDER > 16
        "smlabb  %[res], r1, r3, %[res]  \n"
#else
        "smulbb  %[res], r1, r3          \n"
#endif
        "smlatt  %[res], r1, r3, %[res]  \n"
        "smlabb  %[res], r2, r4, %[res]  \n"
        "smlatt  %[res], r2, r4, %[res]  \n"
        "ldmia   %[s2]!, {r3,r4}         \n"
        SUBHALFREGS(r0, r1, r3)
        SUBHALFREGS(r1, r2, r4)
        "stmia   %[v1]!, {r0,r1}         \n"

        REPEAT_3(
        "ldmia   %[v1],  {r1,r2}         \n"
        "ldmia   %[f2]!, {r3,r4}         \n"
        "smlabb  %[res], r1, r3, %[res]  \n"
        "smlatt  %[res], r1, r3, %[res]  \n"
        "smlabb  %[res], r2, r4, %[res]  \n"
        "smlatt  %[res], r2, r4, %[res]  \n"
        "ldmia   %[s2]!, {r3,r4}         \n"
        SUBHALFREGS(r0, r1, r3)
        SUBHALFREGS(r1, r2, r4)
        "stmia   %[v1]!, {r0,r1}         \n"
        )
#if ORDER > 16
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"
#endif

    "99:                                 \n"
        : /* outputs */
#if ORDER > 16
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [f2] "+r"(f2),
        [s2] "+r"(s2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3", "r4", "r5", "r6", "cc", "memory"
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
        "ldrh    r3, [%[v2]], #2         \n"
#if ORDER > 32
        "mov     r3, r3, lsl #16         \n"
    "1:                                  \n"
        "ldmia   %[v1]!, {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
#else
        "ldmia   %[v1]!, {r0,r1}         \n"
        "smulbb  %[res], r0, r3          \n"
#endif
        "ldmia   %[v2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"

        REPEAT_MLA(
        "ldmia   %[v1]!, {r0,r1}         \n"
        "smlabt  %[res], r0, r3, %[res]  \n"
        "ldmia   %[v2]!, {r2,r3}         \n"
        "smlatb  %[res], r0, r2, %[res]  \n"
        "smlabt  %[res], r1, r2, %[res]  \n"
        "smlatb  %[res], r1, r3, %[res]  \n"
        )
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif
        "b       99f                 \n"

    "20:                                 \n"
    "1:                                  \n"
        "ldmia   %[v1]!, {r0,r1}         \n"
        "ldmia   %[v2]!, {r2,r3}         \n"
#if ORDER > 32
        "smlabb  %[res], r0, r2, %[res]  \n"
#else
        "smulbb  %[res], r0, r2          \n"
#endif
        "smlatt  %[res], r0, r2, %[res]  \n"
        "smlabb  %[res], r1, r3, %[res]  \n"
        "smlatt  %[res], r1, r3, %[res]  \n"

        REPEAT_MLA(
        "ldmia   %[v1]!, {r0,r1}         \n"
        "ldmia   %[v2]!, {r2,r3}         \n"
        "smlabb  %[res], r0, r2, %[res]  \n"
        "smlatt  %[res], r0, r2, %[res]  \n"
        "smlabb  %[res], r1, r3, %[res]  \n"
        "smlatt  %[res], r1, r3, %[res]  \n"
        )
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"  
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
        "r0", "r1", "r2", "r3", "cc", "memory"
    );
    return res;
}
