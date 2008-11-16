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

static inline void vector_add(int16_t* v1, int16_t* v2)
{
#if ORDER > 32
    int order = (ORDER >> 5);
    while (order--)
#endif
    {
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
#if ORDER > 16
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
        *v1++ += *v2++;
#endif
    }
}

static inline void vector_sub(int16_t* v1, int16_t* v2)
{
#if ORDER > 32
    int order = (ORDER >> 5);
    while (order--)
#endif
    {
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
#if ORDER > 16
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
        *v1++ -= *v2++;
#endif
    }
}

/* This version fetches data as 32 bit words, and *requires* v1 to be
 * 32 bit aligned, otherwise it will result either in a data abort, or
 * incorrect results (if ARM aligncheck is disabled). */
static inline int32_t scalarproduct(int16_t* v1, int16_t* v2)
{
    int res = 0;
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

    asm volatile (
        "tst     %[v2], #2               \n"
        "beq     20f                     \n"

    "10:                                 \n"
        "ldrh    r7, [%[v2]], #2         \n"
        "mov     r7, r7, lsl #16         \n"
    "1:                                  \n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "smlabt  %[res], r0, r7, %[res]  \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
        "smlatb  %[res], r0, r4, %[res]  \n"
        "smlabt  %[res], r1, r4, %[res]  \n"
        "smlatb  %[res], r1, r5, %[res]  \n"
        "smlabt  %[res], r2, r5, %[res]  \n"
        "smlatb  %[res], r2, r6, %[res]  \n"
        "smlabt  %[res], r3, r6, %[res]  \n"
        "smlatb  %[res], r3, r7, %[res]  \n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "smlabt  %[res], r0, r7, %[res]  \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
        "smlatb  %[res], r0, r4, %[res]  \n"
        "smlabt  %[res], r1, r4, %[res]  \n"
        "smlatb  %[res], r1, r5, %[res]  \n"
        "smlabt  %[res], r2, r5, %[res]  \n"
        "smlatb  %[res], r2, r6, %[res]  \n"
        "smlabt  %[res], r3, r6, %[res]  \n"
        "smlatb  %[res], r3, r7, %[res]  \n"
#if ORDER > 16
        "subs    %[cnt], %[cnt], #1  \n"
        "bne     1b                  \n"
#endif
        "b       99f                 \n"

    "20:                                 \n"
    "1:                                  \n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
        "smlabb  %[res], r0, r4, %[res]  \n"
        "smlatt  %[res], r0, r4, %[res]  \n"
        "smlabb  %[res], r1, r5, %[res]  \n"
        "smlatt  %[res], r1, r5, %[res]  \n"
        "smlabb  %[res], r2, r6, %[res]  \n"
        "smlatt  %[res], r2, r6, %[res]  \n"
        "smlabb  %[res], r3, r7, %[res]  \n"
        "smlatt  %[res], r3, r7, %[res]  \n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
        "smlabb  %[res], r0, r4, %[res]  \n"
        "smlatt  %[res], r0, r4, %[res]  \n"
        "smlabb  %[res], r1, r5, %[res]  \n"
        "smlatt  %[res], r1, r5, %[res]  \n"
        "smlabb  %[res], r2, r6, %[res]  \n"
        "smlatt  %[res], r2, r6, %[res]  \n"
        "smlabb  %[res], r3, r7, %[res]  \n"
        "smlatt  %[res], r3, r7, %[res]  \n"
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
        [v2] "+r"(v2),
        [res]"+r"(res)
        : /* inputs */
        : /* clobbers */
        "r0", "r1", "r2", "r3",
        "r4", "r5", "r6", "r7"
    );
    return res;
}
