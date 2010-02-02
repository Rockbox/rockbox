/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

ARMv4 vector math copyright (C) 2008 Jens Arnold

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

#if ORDER > 32
#define BLOCK_REPEAT "8"
#elif ORDER > 16
#define BLOCK_REPEAT "7"
#else
#define BLOCK_REPEAT "3"
#endif

/* Calculate scalarproduct, then add a 2nd vector (fused for performance) */
static inline int32_t vector_sp_add(int32_t* v1, int32_t* f2, int32_t* s2)
{
    int res;
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

    asm volatile (
#if ORDER > 32
        "mov     %[res], #0              \n"
    "1:                                  \n"
#else
        "ldmia   %[v1],  {r0-r3}         \n"
        "ldmia   %[f2]!, {r4-r7}         \n"
        "mul     %[res], r4, r0          \n"
        "mla     %[res], r5, r1, %[res]  \n"
        "mla     %[res], r6, r2, %[res]  \n"
        "mla     %[res], r7, r3, %[res]  \n"
        "ldmia   %[s2]!, {r4-r7}         \n"
        "add     r0, r0, r4              \n"
        "add     r1, r1, r5              \n"
        "add     r2, r2, r6              \n"
        "add     r3, r3, r7              \n"
        "stmia   %[v1]!, {r0-r3}         \n"
#endif
        ".rept " BLOCK_REPEAT           "\n"
        "ldmia   %[v1],  {r0-r3}         \n"
        "ldmia   %[f2]!, {r4-r7}         \n"
        "mla     %[res], r4, r0, %[res]  \n"
        "mla     %[res], r5, r1, %[res]  \n"
        "mla     %[res], r6, r2, %[res]  \n"
        "mla     %[res], r7, r3, %[res]  \n"
        "ldmia   %[s2]!, {r4-r7}         \n"
        "add     r0, r0, r4              \n"
        "add     r1, r1, r5              \n"
        "add     r2, r2, r6              \n"
        "add     r3, r3, r7              \n"
        "stmia   %[v1]!, {r0-r3}         \n"
        ".endr                           \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"
#endif
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
        "r5", "r6", "r7", "memory"
    );
    return res;
}

/* Calculate scalarproduct, then subtract a 2nd vector (fused for performance) */
static inline int32_t vector_sp_sub(int32_t* v1, int32_t* f2, int32_t* s2)
{
    int res;
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

    asm volatile (
#if ORDER > 32
        "mov     %[res], #0              \n"
    "1:                                  \n"
#else
        "ldmia   %[v1],  {r0-r3}         \n"
        "ldmia   %[f2]!, {r4-r7}         \n"
        "mul     %[res], r4, r0          \n"
        "mla     %[res], r5, r1, %[res]  \n"
        "mla     %[res], r6, r2, %[res]  \n"
        "mla     %[res], r7, r3, %[res]  \n"
        "ldmia   %[s2]!, {r4-r7}         \n"
        "sub     r0, r0, r4              \n"
        "sub     r1, r1, r5              \n"
        "sub     r2, r2, r6              \n"
        "sub     r3, r3, r7              \n"
        "stmia   %[v1]!, {r0-r3}         \n"
#endif
        ".rept " BLOCK_REPEAT           "\n"
        "ldmia   %[v1],  {r0-r3}         \n"
        "ldmia   %[f2]!, {r4-r7}         \n"
        "mla     %[res], r4, r0, %[res]  \n"
        "mla     %[res], r5, r1, %[res]  \n"
        "mla     %[res], r6, r2, %[res]  \n"
        "mla     %[res], r7, r3, %[res]  \n"
        "ldmia   %[s2]!, {r4-r7}         \n"
        "sub     r0, r0, r4              \n"
        "sub     r1, r1, r5              \n"
        "sub     r2, r2, r6              \n"
        "sub     r3, r3, r7              \n"
        "stmia   %[v1]!, {r0-r3}         \n"
        ".endr                           \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"
#endif
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
        "r5", "r6", "r7", "memory"
    );
    return res;
}

static inline int32_t scalarproduct(int32_t* v1, int32_t* v2)
{
    int res;
#if ORDER > 32
    int cnt = ORDER>>5;
#endif

    asm volatile (
#if ORDER > 32
        "mov     %[res], #0              \n"
    "1:                                  \n"
#else
        "ldmia   %[v1]!, {r0-r3}         \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
        "mul     %[res], r4, r0          \n"
        "mla     %[res], r5, r1, %[res]  \n"
        "mla     %[res], r6, r2, %[res]  \n"
        "mla     %[res], r7, r3, %[res]  \n"
#endif
        ".rept " BLOCK_REPEAT           "\n"
        "ldmia   %[v1]!, {r0-r3}         \n"
        "ldmia   %[v2]!, {r4-r7}         \n"
        "mla     %[res], r4, r0, %[res]  \n"
        "mla     %[res], r5, r1, %[res]  \n"
        "mla     %[res], r6, r2, %[res]  \n"
        "mla     %[res], r7, r3, %[res]  \n"
        ".endr                           \n"
#if ORDER > 32
        "subs    %[cnt], %[cnt], #1      \n"
        "bne     1b                      \n"
#endif
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
        "r4", "r5", "r6", "r7"
    );
    return res;
}
