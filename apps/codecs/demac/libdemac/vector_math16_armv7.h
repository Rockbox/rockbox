/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

ARMv7 neon vector math copyright (C) 2010 Jens Arnold

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
#define REPEAT_BLOCK(x) x x x
#elif ORDER > 16
#define REPEAT_BLOCK(x) x
#else
#define REPEAT_BLOCK(x)
#endif

/* Calculate scalarproduct, then add a 2nd vector (fused for performance) */
static inline int32_t vector_sp_add(int16_t* v1, int16_t* f2, int16_t* s2)
{
    int res;
#if ORDER > 64
    int cnt = ORDER>>6;
#endif

    asm volatile (
#if ORDER > 64
        "vmov.i16    q0, #0              \n"
    "1:                                  \n"
        "subs        %[cnt], %[cnt], #1  \n"
#endif
        "vld1.16     {d6-d9}, [%[f2]]!   \n"
        "vld1.16     {d2-d5}, [%[v1]]    \n"
        "vld1.16     {d10-d13}, [%[s2]]! \n"
#if ORDER > 64
        "vmlal.s16   q0, d2, d6          \n"
#else
        "vmull.s16   q0, d2, d6          \n"
#endif
        "vmlal.s16   q0, d3, d7          \n"
        "vmlal.s16   q0, d4, d8          \n"
        "vmlal.s16   q0, d5, d9          \n"
        "vadd.i16    q1, q1, q5          \n"
        "vadd.i16    q2, q2, q6          \n"
        "vst1.16     {d2-d5}, [%[v1]]!   \n"

        REPEAT_BLOCK(
        "vld1.16     {d6-d9}, [%[f2]]!   \n"
        "vld1.16     {d2-d5}, [%[v1]]    \n"
        "vld1.16     {d10-d13}, [%[s2]]! \n"
        "vmlal.s16   q0, d2, d6          \n"
        "vmlal.s16   q0, d3, d7          \n"
        "vmlal.s16   q0, d4, d8          \n"
        "vmlal.s16   q0, d5, d9          \n"
        "vadd.i16    q1, q1, q5          \n"
        "vadd.i16    q2, q2, q6          \n"
        "vst1.16     {d2-d5}, [%[v1]]!   \n"
        )
#if ORDER > 64
        "bne         1b                  \n"
#endif
        "vpadd.i32   d0, d0, d1          \n"
        "vpaddl.s32  d0, d0              \n"
        "vmov.32     %[res], d0[0]       \n"
        : /* outputs */
#if ORDER > 64
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [f2] "+r"(f2),
        [s2] "+r"(s2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
        "d8", "d9", "d10", "d11", "d12", "d13", "memory"
    );
    return res;
}

/* Calculate scalarproduct, then subtract a 2nd vector (fused for performance) */
static inline int32_t vector_sp_sub(int16_t* v1, int16_t* f2, int16_t* s2)
{
    int res;
#if ORDER > 64
    int cnt = ORDER>>6;
#endif

    asm volatile (
#if ORDER > 64
        "vmov.i16    q0, #0              \n"
    "1:                                  \n"
        "subs        %[cnt], %[cnt], #1  \n"
#endif
        "vld1.16     {d6-d9}, [%[f2]]!   \n"
        "vld1.16     {d2-d5}, [%[v1]]    \n"
        "vld1.16     {d10-d13}, [%[s2]]! \n"
#if ORDER > 64
        "vmlal.s16   q0, d2, d6          \n"
#else
        "vmull.s16   q0, d2, d6          \n"
#endif
        "vmlal.s16   q0, d3, d7          \n"
        "vmlal.s16   q0, d4, d8          \n"
        "vmlal.s16   q0, d5, d9          \n"
        "vsub.i16    q1, q1, q5          \n"
        "vsub.i16    q2, q2, q6          \n"
        "vst1.16     {d2-d5}, [%[v1]]!   \n"

        REPEAT_BLOCK(
        "vld1.16     {d6-d9}, [%[f2]]!   \n"
        "vld1.16     {d2-d5}, [%[v1]]    \n"
        "vld1.16     {d10-d13}, [%[s2]]! \n"
        "vmlal.s16   q0, d2, d6          \n"
        "vmlal.s16   q0, d3, d7          \n"
        "vmlal.s16   q0, d4, d8          \n"
        "vmlal.s16   q0, d5, d9          \n"
        "vsub.i16    q1, q1, q5          \n"
        "vsub.i16    q2, q2, q6          \n"
        "vst1.16     {d2-d5}, [%[v1]]!   \n"
        )
#if ORDER > 64
        "bne         1b                  \n"
#endif
        "vpadd.i32   d0, d0, d1          \n"
        "vpaddl.s32  d0, d0              \n"
        "vmov.32     %[res], d0[0]       \n"
        : /* outputs */
#if ORDER > 64
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [f2] "+r"(f2),
        [s2] "+r"(s2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
        "d8", "d9", "d10", "d11", "d12", "d13", "memory"
    );
    return res;
}

static inline int32_t scalarproduct(int16_t* v1, int16_t* v2)
{
    int res;
#if ORDER > 64
    int cnt = ORDER>>6;
#endif

    asm volatile (
#if ORDER > 64
        "vmov.i16    q0, #0              \n"
    "1:                                  \n"
        "subs        %[cnt], %[cnt], #1  \n"
#endif
        "vld1.16     {d2-d5}, [%[v1]]!   \n"
        "vld1.16     {d6-d9}, [%[v2]]!   \n"
#if ORDER > 64
        "vmlal.s16   q0, d2, d6          \n"
#else
        "vmull.s16   q0, d2, d6          \n"
#endif
        "vmlal.s16   q0, d3, d7          \n"
        "vmlal.s16   q0, d4, d8          \n"
        "vmlal.s16   q0, d5, d9          \n"

        REPEAT_BLOCK(
        "vld1.16     {d2-d5}, [%[v1]]!   \n"
        "vld1.16     {d6-d9}, [%[v2]]!   \n"
        "vmlal.s16   q0, d2, d6          \n"
        "vmlal.s16   q0, d3, d7          \n"
        "vmlal.s16   q0, d4, d8          \n"
        "vmlal.s16   q0, d5, d9          \n"
        )
#if ORDER > 64
        "bne         1b                  \n"
#endif
        "vpadd.i32   d0, d0, d1          \n"
        "vpaddl.s32  d0, d0              \n"
        "vmov.32     %[res], d0[0]       \n"
        : /* outputs */
#if ORDER > 64
        [cnt]"+r"(cnt),
#endif
        [v1] "+r"(v1),
        [v2] "+r"(v2),
        [res]"=r"(res)
        : /* inputs */
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4",
        "d5", "d6", "d7", "d8", "d9"
    );
    return res;
}
