/*

libdemac - A Monkey's Audio decoder

$Id$

Copyright (C) Dave Chapman 2007

Coldfire vector math copyright (C) 2007 Jens Arnold

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
#define ADDHALFREGS(s1, sum)            /* 's1' can be an A or D reg */    \
        "move.l " #s1  ",   %%d4  \n"   /* 'sum' must be a D reg */        \
        "add.l  " #sum ", " #s1  "\n"   /* 's1' and %%d4 are clobbered! */ \
        "clr.w    %%d4            \n"   \
        "add.l    %%d4  , " #sum "\n"   \
        "move.w " #s1  ", " #sum "\n"

    asm volatile (
#if ORDER > 16
        "moveq.l %[cnt], %%d5        \n"
    "1:                              \n"
#endif
        "movem.l (%[v1]), %%d0-%%d3  \n"
        "movem.l (%[v2]), %%a0-%%a3  \n"
        
        ADDHALFREGS(%%a0, %%d0)
        ADDHALFREGS(%%a1, %%d1)
        ADDHALFREGS(%%a2, %%d2)
        ADDHALFREGS(%%a3, %%d3)

        "movem.l %%d0-%%d3, (%[v1])  \n"
        "lea.l   (16, %[v1]), %[v1]  \n"
        "movem.l (%[v1]), %%d0-%%d3  \n"
        "lea.l   (16, %[v2]), %[v2]  \n"
        "movem.l (%[v2]), %%a0-%%a3  \n"

        ADDHALFREGS(%%a0, %%d0)
        ADDHALFREGS(%%a1, %%d1)
        ADDHALFREGS(%%a2, %%d2)
        ADDHALFREGS(%%a3, %%d3)

        "movem.l %%d0-%%d3, (%[v1])  \n"
#if ORDER > 16
        "lea.l   (16, %[v1]), %[v1]  \n"
        "lea.l   (16, %[v2]), %[v2]  \n"
        "subq.l  #1, %%d5            \n"
        "bne.s   1b                  \n"
#endif
        : /* outputs */
        [v1]"+a"(v1),
        [v2]"+a"(v2)
        : /* inputs */
        [cnt]"n"(ORDER>>4)
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4", "d5",
        "a0", "a1", "a2", "a3", "memory"
    );
}

static inline void vector_sub(int16_t* v1, int16_t* v2)
{
#define SUBHALFREGS(min, sub, dif)      /* 'min' can be an A or D reg */     \
        "move.l " #min ", " #dif "\n"   /* 'sub' and 'dif' must be D regs */ \
        "sub.l  " #sub ", " #min "\n"   /* 'min' and 'sub' are clobbered! */ \
        "clr.w  " #sub           "\n"   \
        "sub.l  " #sub ", " #dif "\n"   \
        "move.w " #min ", " #dif "\n" 

    asm volatile (
#if ORDER > 16
        "moveq.l %[cnt], %%d5        \n"
    "1:                              \n"
#endif
        "movem.l (%[v1]), %%a0-%%a3  \n"
        "movem.l (%[v2]), %%d1-%%d4  \n"
        
        SUBHALFREGS(%%a0, %%d1, %%d0)
        SUBHALFREGS(%%a1, %%d2, %%d1)
        SUBHALFREGS(%%a2, %%d3, %%d2)
        SUBHALFREGS(%%a3, %%d4, %%d3)

        "movem.l %%d0-%%d3, (%[v1])  \n"
        "lea.l   (16, %[v1]), %[v1]  \n"
        "movem.l (%[v1]), %%a0-%%a3  \n"
        "lea.l   (16, %[v2]), %[v2]  \n"
        "movem.l (%[v2]), %%d1-%%d4  \n"
            
        SUBHALFREGS(%%a0, %%d1, %%d0)
        SUBHALFREGS(%%a1, %%d2, %%d1)
        SUBHALFREGS(%%a2, %%d3, %%d2)
        SUBHALFREGS(%%a3, %%d4, %%d3)

        "movem.l %%d0-%%d3, (%[v1])  \n"
#if ORDER > 16
        "lea.l   (16, %[v1]), %[v1]  \n"
        "lea.l   (16, %[v2]), %[v2]  \n"
        "subq.l  #1, %%d5            \n"
        "bne.s   1b                  \n"
#endif
        : /* outputs */
        [v1]"+a"(v1),
        [v2]"+a"(v2)
        : /* inputs */
        [cnt]"n"(ORDER>>4)
        : /* clobbers */
        "d0", "d1", "d2", "d3", "d4", "d5",
        "a0", "a1", "a2", "a3", "memory"
    );
}

#define PREPARE_SCALARPRODUCT coldfire_set_macsr(0); /* signed integer mode */

/* Needs EMAC in signed integer mode! */
static inline int32_t scalarproduct(int16_t* v1, int16_t* v2)
{
    int res = 0;

#define MACBLOCK4                                        \
        "mac.w   %%d0u, %%d1u, (%[v1])+, %%d2, %%acc0\n" \
        "mac.w   %%d0l, %%d1l, (%[v2])+, %%d3, %%acc0\n" \
        "mac.w   %%d2u, %%d3u, (%[v1])+, %%d0, %%acc0\n" \
        "mac.w   %%d2l, %%d3l, (%[v2])+, %%d1, %%acc0\n"

    asm volatile (
#if ORDER > 32
        "moveq.l %[cnt], %[res]                      \n"
#endif
        "move.l  (%[v1])+, %%d0                      \n"
        "move.l  (%[v2])+, %%d1                      \n"
    "1:                                              \n"
#if ORDER > 16
        MACBLOCK4
        MACBLOCK4
        MACBLOCK4
        MACBLOCK4
#endif
        MACBLOCK4
        MACBLOCK4
        MACBLOCK4
        "mac.w   %%d0u, %%d1u, (%[v1])+, %%d2, %%acc0\n"
        "mac.w   %%d0l, %%d1l, (%[v2])+, %%d3, %%acc0\n"
#if ORDER > 32
        "mac.w   %%d2u, %%d3u, (%[v1])+, %%d0, %%acc0\n"
        "mac.w   %%d2l, %%d3l, (%[v2])+, %%d1, %%acc0\n"
        "subq.l  #1, %[res]                          \n"
        "bne.w   1b                                  \n"
#else
        "mac.w   %%d2u, %%d3u, %%acc0                \n"
        "mac.w   %%d2l, %%d3l, %%acc0                \n"
#endif
        "movclr.l %%acc0, %[res]                     \n"
        : /* outputs */
        [v1]"+a"(v1),
        [v2]"+a"(v2),
        [res]"=&d"(res)
        : /* inputs */
        [cnt]"n"(ORDER>>5)
        : /* clobbers */
        "d0", "d1", "d2", "d3"
    );
    return res;
}
