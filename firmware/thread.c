/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Ulf Ralberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "thread.h"

typedef union
{
    struct regs_t
    {
        unsigned int  r[7]; /* Registers r8 thru r14 */
        void          *sp;  /* Stack pointer (r15) */
        unsigned int  sr;   /* Status register */
#if 0
        void*         gbr;  /* Global base register */
#endif
        void*         pr;   /* Procedure register */
    } regs;
    unsigned int mem[32];
} ctx_t;

typedef struct
{
    int   created;
    int   current;
    ctx_t ctx[MAXTHREADS] __attribute__ ((aligned (32)));
} thread_t;

static thread_t threads = {1, 0};

/*--------------------------------------------------------------------------- 
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void stctx(void* addr)
{
    unsigned int tmp;

    /*
      [Alkorr] sorry, this code is totally wrong.
      
      Why ?

        "mov.l %0,@(imm,%1)"

      must be interpreted as :
      
        "%0 = ((long *)%1)[imm]"  
      
      not as :
      
        "%0 = *((long *)(((char *)%1) + imm))"

      real offset = "imm" x 1 if byte access (.b)
                  = "imm" x 2 if 16-bit word access (.w)
                  = "imm" x 4 if 32-bit word access (.l)

      Don't forget, SH doesn't like misaligned address, so
      remember it doesn't make any sense to have an odd
      offset ;).

    */

#if 0
    asm volatile ("mov.l r8, @(0, %1)\n\t"
                  "mov.l r9, @(4, %1)\n\t"
                  "mov.l r10, @(8, %1)\n\t"
                  "mov.l r11, @(12, %1)\n\t"
                  "add #16,%1\n\t"
                  "mov.l r12, @(0, %1)\n\t"
                  "mov.l r13, @(4, %1)\n\t"
                  "mov.l r14, @(8, %1)\n\t"
                  "mov.l r15, @(12, %1)\n\t"
                  "add #16,%1\n\t"
                  "stc sr, %0\n\t"
                  "mov.l %0, @(0, %1)\n\t"
                  "stc gbr, %0\n\t"
                  "mov.l %0, @(4, %1)\n\t"
                  "sts pr, %0\n\t"
                  "mov.l %0, @(8, %1)" : "=r&" (tmp) : "r" (addr));
#endif
#if 0
    /* here the right code */
    asm volatile ("mov.l r8,  @(0,%1)\n\t"
                  "mov.l r9,  @(1,%1)\n\t"
                  "mov.l r10, @(2,%1)\n\t"
                  "mov.l r11, @(3,%1)\n\t"
                  "mov.l r12, @(4,%1)\n\t"
                  "mov.l r13, @(5,%1)\n\t"
                  "mov.l r14, @(6,%1)\n\t"
                  "mov.l r15, @(7,%1)\n\t"
                  "stc.l sr,  %0\n\t"
                  "mov.l %0,  @(8,%1)\n\t"
                  "sts   pr,  %0\n\t"
                  "mov.l %0,  @(9,%1)" : "=r&" (tmp) : "r" (addr));
#endif
    /* here a far better code */
    asm volatile ("sts.l pr,  @-%0\n\t"
                  "stc.l sr,  @-%0\n\t"
                  "mov.l r15, @-%0\n\t"
                  "mov.l r14, @-%0\n\t"
                  "mov.l r13, @-%0\n\t"
                  "mov.l r12, @-%0\n\t"
                  "mov.l r11, @-%0\n\t"
                  "mov.l r10, @-%0\n\t"
                  "mov.l r9,  @-%0\n\t"
                  "mov.l r8,  @-%0" : : "r" (addr+4*10));
   
}

/*--------------------------------------------------------------------------- 
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void ldctx(void* addr)
{
    unsigned int tmp;
    
    /* same remarks than above */

#if 0
    asm volatile ("mov.l @(0, %1), r8\n\t"
                  "mov.l @(4, %1), r9\n\t"
                  "mov.l @(8, %1), r10\n\t"
                  "mov.l @(12, %1), r11\n\t"
                  "add #16,%1\n\t"
                  "mov.l @(0, %1), r12\n\t"
                  "mov.l @(4, %1), r13\n\t"
                  "mov.l @(8, %1), r14\n\t"
                  "mov.l @(12, %1), r15\n\t"
                  "add #16,%1\n\t"
                  "mov.l @(0, %1), r0\n\t"
                  "ldc %0, sr\n\t"
                  "mov.l @(4, %1), %0\n\t"
                  "ldc %0, gbr\n\t"
                  "mov.l @(8, %1), %0\n\t"
                  "lds %0, pr\n\t"
                  "mov.l %0, @(0, r15)" : "=r&" (tmp) : "r" (addr));
#endif
    asm volatile ("mov.l @%0+,r8\n\t"
                  "mov.l @%0+,r9\n\t"
                  "mov.l @%0+,r10\n\t"
                  "mov.l @%0+,r11\n\t"
                  "mov.l @%0+,r12\n\t"
                  "mov.l @%0+,r13\n\t"
                  "mov.l @%0+,r14\n\t"
                  "mov.l @%0+,r15\n\t"
                  "ldc.l @%0+,sr\n\t"
                  "lds.l @%0+,pr" : : "r" (addr));

}

/*--------------------------------------------------------------------------- 
 * Switch thread in round robin fashion.
 *---------------------------------------------------------------------------
 */
void
switch_thread(void)
{
    int        ct;
    int        nt;
    thread_t*  t = &threads;

    nt = ct = t->current;
    if (++nt >= t->created)
        nt = 0;
    t->current = nt;
    stctx(&t->ctx[ct]);
    ldctx(&t->ctx[nt]);
}

/*--------------------------------------------------------------------------- 
 * Create thread. Stack is aligned at 32 byte boundary to fit cache line.
 * > 220 bytes allocated on top for exception handlers as per EABI spec.
 * Return 0 if context area could be allocated, else -1.
 *---------------------------------------------------------------------------
 */
int create_thread(void* fp, void* sp, int stk_size)
{
    thread_t* t = &threads;

    if (t->created >= MAXTHREADS)
        return -1;
    else
    {
        ctx_t* ctxp = &t->ctx[t->created++];
        stctx(ctxp);
        ctxp->regs.sp = (void*)(((unsigned int)sp + stk_size) & ~31);
        ctxp->regs.pr = fp;
    }
    return 0;
}
