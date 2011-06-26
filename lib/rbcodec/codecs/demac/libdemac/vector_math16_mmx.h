/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

MMX vector math copyright (C) 2010 Jens Arnold

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

#define REPEAT_MB3(x, n) x(n) x(n+8) x(n+16)
#define REPEAT_MB7(x, n) x(n) x(n+8) x(n+16) x(n+24) x(n+32) x(n+40) x(n+48)
#define REPEAT_MB8(x, n) REPEAT_MB7(x, n) x(n+56)

#if ORDER == 16     /* 3 times */
#define REPEAT_MB(x) REPEAT_MB3(x, 8) 
#elif ORDER == 32   /* 7 times */
#define REPEAT_MB(x) REPEAT_MB7(x, 8) 
#elif ORDER == 64   /* 5*3 == 15 times */
#define REPEAT_MB(x) REPEAT_MB3(x,  8) REPEAT_MB3(x, 32) REPEAT_MB3(x, 56) \
                     REPEAT_MB3(x, 80) REPEAT_MB3(x, 104)
#elif ORDER == 256  /* 9*7 == 63 times */
#define REPEAT_MB(x) REPEAT_MB7(x,   8) REPEAT_MB7(x,  64) REPEAT_MB7(x, 120) \
                     REPEAT_MB7(x, 176) REPEAT_MB7(x, 232) REPEAT_MB7(x, 288) \
                     REPEAT_MB7(x, 344) REPEAT_MB7(x, 400) REPEAT_MB7(x, 456)
#elif ORDER == 1280 /* 8*8 == 64 times */
#define REPEAT_MB(x) REPEAT_MB8(x,   0) REPEAT_MB8(x,  64) REPEAT_MB8(x, 128) \
                     REPEAT_MB8(x, 192) REPEAT_MB8(x, 256) REPEAT_MB8(x, 320) \
                     REPEAT_MB8(x, 384) REPEAT_MB8(x, 448)
#else
#error unsupported order
#endif


static inline int32_t vector_sp_add(int16_t* v1, int16_t* f2, int16_t *s2)
{
    int res, t;
#if ORDER > 256
    int cnt = ORDER>>8;
#endif

    asm volatile (
#if ORDER > 256
        "pxor    %%mm2, %%mm2        \n"
    "1:                              \n"
#else
        "movq    (%[v1]), %%mm2      \n"
        "movq    %%mm2, %%mm0        \n"
        "pmaddwd (%[f2]), %%mm2      \n"
        "paddw   (%[s2]), %%mm0      \n"
        "movq    %%mm0, (%[v1])      \n"
#endif

#define SP_ADD_BLOCK(n)                      \
        "movq    " #n "(%[v1]), %%mm1    \n" \
        "movq    %%mm1, %%mm0            \n" \
        "pmaddwd " #n "(%[f2]), %%mm1    \n" \
        "paddw   " #n "(%[s2]), %%mm0    \n" \
        "movq    %%mm0, " #n "(%[v1])    \n" \
        "paddd   %%mm1, %%mm2            \n"
        
REPEAT_MB(SP_ADD_BLOCK)

#if ORDER > 256
        "add     $512, %[v1]         \n"
        "add     $512, %[s2]         \n"
        "add     $512, %[f2]         \n"
        "dec     %[cnt]              \n"
        "jne     1b                  \n"
#endif

        "movd    %%mm2, %[t]         \n"
        "psrlq   $32, %%mm2          \n"
        "movd    %%mm2, %[res]       \n"
        "add     %[t], %[res]        \n"
        : /* outputs */
#if ORDER > 256
        [cnt]"+r"(cnt),
        [s2] "+r"(s2),
        [res]"=r"(res),
        [t]  "=r"(t)
        : /* inputs */
        [v1]"2"(v1),
        [f2]"3"(f2)
#else
        [res]"=r"(res),
        [t]  "=r"(t)
        : /* inputs */
        [v1]"r"(v1),
        [f2]"r"(f2),
        [s2]"r"(s2)
#endif
        : /* clobbers */
        "mm0", "mm1", "mm2"
    );
    return res;
}

static inline int32_t vector_sp_sub(int16_t* v1, int16_t* f2, int16_t *s2)
{
    int res, t;
#if ORDER > 256
    int cnt = ORDER>>8;
#endif

    asm volatile (
#if ORDER > 256
        "pxor    %%mm2, %%mm2        \n"
    "1:                              \n"
#else
        "movq    (%[v1]), %%mm2      \n"
        "movq    %%mm2, %%mm0        \n"
        "pmaddwd (%[f2]), %%mm2      \n"
        "psubw   (%[s2]), %%mm0      \n"
        "movq    %%mm0, (%[v1])      \n"
#endif

#define SP_SUB_BLOCK(n)                      \
        "movq    " #n "(%[v1]), %%mm1    \n" \
        "movq    %%mm1, %%mm0            \n" \
        "pmaddwd " #n "(%[f2]), %%mm1    \n" \
        "psubw   " #n "(%[s2]), %%mm0    \n" \
        "movq    %%mm0, " #n "(%[v1])    \n" \
        "paddd   %%mm1, %%mm2            \n"

REPEAT_MB(SP_SUB_BLOCK)

#if ORDER > 256
        "add     $512, %[v1]         \n"
        "add     $512, %[s2]         \n"
        "add     $512, %[f2]         \n"
        "dec     %[cnt]              \n"
        "jne     1b                  \n"
#endif

        "movd    %%mm2, %[t]         \n"
        "psrlq   $32, %%mm2          \n"
        "movd    %%mm2, %[res]       \n"
        "add     %[t], %[res]        \n"
        : /* outputs */
#if ORDER > 256
        [cnt]"+r"(cnt),
        [s2] "+r"(s2),
        [res]"=r"(res),
        [t]  "=r"(t)
        : /* inputs */
        [v1]"2"(v1),
        [f2]"3"(f2)
#else
        [res]"=r"(res),
        [t]  "=r"(t)
        : /* inputs */
        [v1]"r"(v1),
        [f2]"r"(f2),
        [s2]"r"(s2)
#endif
        : /* clobbers */
        "mm0", "mm1", "mm2"
    );
    return res;
}

static inline int32_t scalarproduct(int16_t* v1, int16_t* v2)
{
    int res, t;
#if ORDER > 256
    int cnt = ORDER>>8;
#endif
               
    asm volatile (
#if ORDER > 256
        "pxor    %%mm1, %%mm1        \n"
    "1:                              \n"
#else
        "movq    (%[v1]), %%mm1      \n"
        "pmaddwd (%[v2]), %%mm1      \n"
#endif

#define SP_BLOCK(n)                          \
        "movq    " #n "(%[v1]), %%mm0    \n" \
        "pmaddwd " #n "(%[v2]), %%mm0    \n" \
        "paddd   %%mm0, %%mm1            \n"

REPEAT_MB(SP_BLOCK)

#if ORDER > 256
        "add     $512, %[v1]         \n"
        "add     $512, %[v2]         \n"
        "dec     %[cnt]              \n"
        "jne     1b                  \n"
#endif

        "movd    %%mm1, %[t]         \n"
        "psrlq   $32, %%mm1          \n"
        "movd    %%mm1, %[res]       \n"
        "add     %[t], %[res]        \n"
        : /* outputs */
#if ORDER > 256
        [cnt]"+r"(cnt),
        [res]"=r"(res),
        [t]  "=r"(t)
        : /* inputs */
        [v1]"1"(v1),
        [v2]"2"(v2)
#else
        [res]"=r"(res),
        [t]  "=r"(t)
        : /* inputs */
        [v1]"r"(v1),
        [v2]"r"(v2)
#endif
        : /* clobbers */
        "mm0", "mm1"
    );
    return res;
}
