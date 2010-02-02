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

#define FUSED_VECTOR_MATH

#define PREPARE_SCALARPRODUCT coldfire_set_macsr(0); /* signed integer mode */

/* Calculate scalarproduct, then add a 2nd vector (fused for performance)
 * This version fetches data as 32 bit words, and *recommends* v1 to be
 * 32 bit aligned. It also assumes that f2 and s2 are either both 32 bit
 * aligned or both unaligned. Performance will suffer if either condition
 * isn't met. It also needs EMAC in signed integer mode. */
static inline int32_t vector_sp_add(int16_t* v1, int16_t* f2, int16_t* s2)
{
    int res;
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

#define ADDHALFREGS(s1, s2, sum)       /* Add register halves straight. */  \
        "move.l " #s1  ", " #sum "\n"  /* 's1' and 's2' can be A or D */    \
        "add.l  " #s2  ", " #s1  "\n"  /* regs, 'sum' must be a D reg. */   \
        "clr.w  " #sum "          \n"  /* 's1' is clobbered! */             \
        "add.l  " #s2  ", " #sum "\n"  \
        "move.w " #s1  ", " #sum "\n"
        
#define ADDHALFXREGS(s1, s2, sum)      /* Add register halves across. */    \
        "clr.w  " #sum "          \n"  /* Needs 'sum' pre-swapped, swaps */ \
        "add.l  " #s1  ", " #sum "\n"  /* 's2', and clobbers 's1'. */       \
        "swap   " #s2  "          \n"  /* 's1' can be an A or D reg. */     \
        "add.l  " #s2  ", " #s1  "\n"  /* 'sum' and 's2' must be D regs. */ \
        "move.w " #s1  ", " #sum "\n"

    asm volatile (
        "move.l  %[f2], %%d0                         \n"
        "and.l   #2, %%d0                            \n"
        "jeq     20f                                 \n"

    "10:                                             \n"
        "move.w  (%[f2])+, %%d0                      \n"
        "move.w  (%[s2])+, %%d1                      \n"
        "swap    %%d1                                \n"
    "1:                                              \n"
        ".rept   2                                   \n"
        "movem.l (%[v1]), %%d6-%%d7/%%a0-%%a1        \n"
        "mac.w   %%d0l, %%d6u, (%[f2])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%d6l, (%[s2])+, %%d2, %%acc0\n"
        ADDHALFXREGS(%%d6, %%d2, %%d1)
        "mac.w   %%d0l, %%d7u, (%[f2])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%d7l, (%[s2])+, %%d6, %%acc0\n"
        "move.l  %%d1, (%[v1])+                      \n"
        ADDHALFXREGS(%%d7, %%d6, %%d2)
        "mac.w   %%d0l, %%a0u, (%[f2])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%a0l, (%[s2])+, %%d7, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        ADDHALFXREGS(%%a0, %%d7, %%d6)
        "mac.w   %%d0l, %%a1u, (%[f2])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%a1l, (%[s2])+, %%d1, %%acc0\n"
        "move.l  %%d6, (%[v1])+                      \n"
        ADDHALFXREGS(%%a1, %%d1, %%d7)
        "move.l  %%d7, (%[v1])+                      \n"
        ".endr                                       \n"

#if ORDER > 16
        "subq.l  #1, %[res]                          \n"
        "bne.w   1b                                  \n"
#endif
        "jra     99f                                 \n"

    "20:                                             \n"
        "move.l  (%[f2])+, %%d0                      \n"
    "1:                                              \n"
        "movem.l (%[v1]), %%d6-%%d7/%%a0-%%a1        \n"
        "mac.w   %%d0u, %%d6u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d6l, (%[f2])+, %%d0, %%acc0\n"
        ADDHALFREGS(%%d6, %%d1, %%d2)
        "mac.w   %%d0u, %%d7u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d7l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        ADDHALFREGS(%%d7, %%d1, %%d2)
        "mac.w   %%d0u, %%a0u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%a0l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        ADDHALFREGS(%%a0, %%d1, %%d2)
        "mac.w   %%d0u, %%a1u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%a1l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        ADDHALFREGS(%%a1, %%d1, %%d2)
        "move.l  %%d2, (%[v1])+                      \n"

        "movem.l (%[v1]), %%d6-%%d7/%%a0-%%a1        \n"
        "mac.w   %%d0u, %%d6u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d6l, (%[f2])+, %%d0, %%acc0\n"
        ADDHALFREGS(%%d6, %%d1, %%d2)
        "mac.w   %%d0u, %%d7u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d7l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        ADDHALFREGS(%%d7, %%d1, %%d2)
        "mac.w   %%d0u, %%a0u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%a0l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        ADDHALFREGS(%%a0, %%d1, %%d2)
        "mac.w   %%d0u, %%a1u, (%[s2])+, %%d1, %%acc0\n"
#if ORDER > 16
        "mac.w   %%d0l, %%a1l, (%[f2])+, %%d0, %%acc0\n"
#else
        "mac.w   %%d0l, %%a1l, %%acc0                \n"
#endif
        "move.l  %%d2, (%[v1])+                      \n"
        ADDHALFREGS(%%a1, %%d1, %%d2)
        "move.l  %%d2, (%[v1])+                      \n"
#if ORDER > 16
        "subq.l  #1, %[res]                          \n"
        "bne.w   1b                                  \n"
#endif

    "99:                                             \n"
        "movclr.l %%acc0, %[res]                     \n"
        : /* outputs */
        [v1]"+a"(v1),
        [f2]"+a"(f2),
        [s2]"+a"(s2),
        [res]"=d"(res)
        : /* inputs */
#if ORDER > 16
        [cnt]"[res]"(cnt)
#endif
        : /* clobbers */
        "d0", "d1", "d2", "d6", "d7", 
        "a0", "a1", "memory"

    );
    return res;
}

/* Calculate scalarproduct, then subtract a 2nd vector (fused for performance)
 * This version fetches data as 32 bit words, and *recommends* v1 to be
 * 32 bit aligned. It also assumes that f2 and s2 are either both 32 bit
 * aligned or both unaligned. Performance will suffer if either condition
 * isn't met. It also needs EMAC in signed integer mode. */
static inline int32_t vector_sp_sub(int16_t* v1, int16_t* f2, int16_t* s2)
{
    int res;
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

#define SUBHALFREGS(min, sub, dif)    /* Subtract register halves straight. */ \
        "move.l " #min ", " #dif "\n" /* 'min' can be an A or D reg */         \
        "sub.l  " #sub ", " #min "\n" /* 'sub' and 'dif' must be D regs */     \
        "clr.w  " #sub           "\n" /* 'min' and 'sub' are clobbered! */     \
        "sub.l  " #sub ", " #dif "\n" \
        "move.w " #min ", " #dif "\n" 
        
#define SUBHALFXREGS(min, s2, s1d)    /* Subtract register halves across. */ \
        "clr.w  " #s1d           "\n" /* Needs 's1d' pre-swapped, swaps */   \
        "sub.l  " #s1d ", " #min "\n" /* 's2' and clobbers 'min'. */         \
        "move.l " #min ", " #s1d "\n" /* 'min' can be an A or D reg, */      \
        "swap   " #s2            "\n" /* 's2' and 's1d' must be D regs. */   \
        "sub.l  " #s2  ", " #min "\n" \
        "move.w " #min ", " #s1d "\n"

    asm volatile (
        "move.l  %[f2], %%d0                         \n"
        "and.l   #2, %%d0                            \n"
        "jeq     20f                                 \n"

    "10:                                             \n"
        "move.w  (%[f2])+, %%d0                      \n"
        "move.w  (%[s2])+, %%d1                      \n"
        "swap    %%d1                                \n"
    "1:                                              \n"
        ".rept   2                                   \n"
        "movem.l (%[v1]), %%d6-%%d7/%%a0-%%a1        \n"
        "mac.w   %%d0l, %%d6u, (%[f2])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%d6l, (%[s2])+, %%d2, %%acc0\n"
        SUBHALFXREGS(%%d6, %%d2, %%d1)
        "mac.w   %%d0l, %%d7u, (%[f2])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%d7l, (%[s2])+, %%d6, %%acc0\n"
        "move.l  %%d1, (%[v1])+                      \n"
        SUBHALFXREGS(%%d7, %%d6, %%d2)
        "mac.w   %%d0l, %%a0u, (%[f2])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%a0l, (%[s2])+, %%d7, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        SUBHALFXREGS(%%a0, %%d7, %%d6)
        "mac.w   %%d0l, %%a1u, (%[f2])+, %%d0, %%acc0\n"
        "mac.w   %%d0u, %%a1l, (%[s2])+, %%d1, %%acc0\n"
        "move.l  %%d6, (%[v1])+                      \n"
        SUBHALFXREGS(%%a1, %%d1, %%d7)
        "move.l  %%d7, (%[v1])+                      \n"
        ".endr                                       \n"

#if ORDER > 16
        "subq.l  #1, %[res]                          \n"
        "bne.w   1b                                  \n"
#endif

        "jra     99f                                 \n"

    "20:                                             \n"
        "move.l  (%[f2])+, %%d0                      \n"
    "1:                                              \n"
        "movem.l (%[v1]), %%d6-%%d7/%%a0-%%a1        \n"
        "mac.w   %%d0u, %%d6u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d6l, (%[f2])+, %%d0, %%acc0\n"
        SUBHALFREGS(%%d6, %%d1, %%d2)
        "mac.w   %%d0u, %%d7u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d7l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        SUBHALFREGS(%%d7, %%d1, %%d2)
        "mac.w   %%d0u, %%a0u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%a0l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        SUBHALFREGS(%%a0, %%d1, %%d2)
        "mac.w   %%d0u, %%a1u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%a1l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        SUBHALFREGS(%%a1, %%d1, %%d2)
        "move.l  %%d2, (%[v1])+                      \n"

        "movem.l (%[v1]), %%d6-%%d7/%%a0-%%a1        \n"
        "mac.w   %%d0u, %%d6u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d6l, (%[f2])+, %%d0, %%acc0\n"
        SUBHALFREGS(%%d6, %%d1, %%d2)
        "mac.w   %%d0u, %%d7u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d7l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        SUBHALFREGS(%%d7, %%d1, %%d2)
        "mac.w   %%d0u, %%a0u, (%[s2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%a0l, (%[f2])+, %%d0, %%acc0\n"
        "move.l  %%d2, (%[v1])+                      \n"
        SUBHALFREGS(%%a0, %%d1, %%d2)
        "mac.w   %%d0u, %%a1u, (%[s2])+, %%d1, %%acc0\n"
#if ORDER > 16
        "mac.w   %%d0l, %%a1l, (%[f2])+, %%d0, %%acc0\n"
#else
        "mac.w   %%d0l, %%a1l, %%acc0                \n"
#endif
        "move.l  %%d2, (%[v1])+                      \n"
        SUBHALFREGS(%%a1, %%d1, %%d2)
        "move.l  %%d2, (%[v1])+                      \n"
#if ORDER > 16
        "subq.l  #1, %[res]                          \n"
        "bne.w   1b                                  \n"
#endif

    "99:                                             \n"
        "movclr.l %%acc0, %[res]                     \n"
        : /* outputs */
        [v1]"+a"(v1),
        [f2]"+a"(f2),
        [s2]"+a"(s2),
        [res]"=d"(res)
        : /* inputs */
#if ORDER > 16
        [cnt]"[res]"(cnt)
#endif
        : /* clobbers */
        "d0", "d1", "d2", "d6", "d7", 
        "a0", "a1", "memory"

    );
    return res;
}

/* This version fetches data as 32 bit words, and *recommends* v1 to be
 * 32 bit aligned, otherwise performance will suffer. It also needs EMAC
 * in signed integer mode. */
static inline int32_t scalarproduct(int16_t* v1, int16_t* v2)
{
    int res;
#if ORDER > 16
    int cnt = ORDER>>4;
#endif

    asm volatile (
        "move.l  %[v2], %%d0                         \n"
        "and.l   #2, %%d0                            \n"
        "jeq     20f                                 \n"

    "10:                                             \n"
        "move.l  (%[v1])+, %%d0                      \n"
        "move.w  (%[v2])+, %%d1                      \n"
    "1:                                              \n"
        ".rept   7                                   \n"
        "mac.w   %%d0u, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        "mac.w   %%d0l, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        ".endr                                       \n"

        "mac.w   %%d0u, %%d1l, (%[v2])+, %%d1, %%acc0\n"
#if ORDER > 16
        "mac.w   %%d0l, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        "subq.l  #1, %[res]                          \n"
        "bne.b   1b                                  \n"
#else
        "mac.w   %%d0l, %%d1u, %%acc0                \n"
#endif
        "jra     99f                                  \n"
        
    "20:                                             \n"
        "move.l  (%[v1])+, %%d0                      \n"
        "move.l  (%[v2])+, %%d1                      \n"
    "1:                                              \n"
        ".rept   3                                   \n"
        "mac.w   %%d0u, %%d1u, (%[v1])+, %%d2, %%acc0\n"
        "mac.w   %%d0l, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        "mac.w   %%d2u, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        "mac.w   %%d2l, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        ".endr                                       \n"

        "mac.w   %%d0u, %%d1u, (%[v1])+, %%d2, %%acc0\n"
        "mac.w   %%d0l, %%d1l, (%[v2])+, %%d1, %%acc0\n"
#if ORDER > 16
        "mac.w   %%d2u, %%d1u, (%[v1])+, %%d0, %%acc0\n"
        "mac.w   %%d2l, %%d1l, (%[v2])+, %%d1, %%acc0\n"
        "subq.l  #1, %[res]                          \n"
        "bne.b   1b                                  \n"
#else
        "mac.w   %%d2u, %%d1u, %%acc0                \n"
        "mac.w   %%d2l, %%d1l, %%acc0                \n"
#endif

    "99:                                             \n"
        "movclr.l %%acc0, %[res]                     \n"
        : /* outputs */
        [v1]"+a"(v1),
        [v2]"+a"(v2),
        [res]"=d"(res)
        : /* inputs */
#if ORDER > 16
        [cnt]"[res]"(cnt)
#endif
        : /* clobbers */
        "d0", "d1", "d2"
    );
    return res;
}
