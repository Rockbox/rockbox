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
      void*         gbr;  /* Global base register */
      void*         pr;   /* Procedure register */
   } regs;
   unsigned int mem[32];
} ctx_t;

typedef struct
{
   int		created;
   int		current;
   ctx_t ctx[MAXTHREADS] __attribute__ ((aligned (32)));
} thread_t;

static thread_t threads = {1, 0};

/*--------------------------------------------------------------------------- 
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void stctx(void* addr)
{
   asm volatile ("mov.l r8, @(0, %0)\n\t"
		 "mov.l r9, @(4, %0)\n\t"
		 "mov.l r10, @(8, %0)\n\t"
		 "mov.l r11, @(12, %0)\n\t"
		 "mov.l r12, @(16, %0)\n\t"
		 "mov.l r13, @(20, %0)\n\t"
		 "mov.l r14, @(24, %0)\n\t"
		 "mov.l r15, @(28, %0)\n\t"
		 "stc sr, r0\n\t"
		 "mov.l r0, @(32, %0)\n\t"
		 "stc gbr, r0\n\t"
		 "mov.l r0, @(36, %0)\n\t"
		 "sts pr, r0\n\t"
		 "mov.l r0, @(40, %0)" :: "r" (addr));
}

/*--------------------------------------------------------------------------- 
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void ldctx(void* addr)
{
   asm volatile ("mov.l @(0, %0), r8\n\t"
		 "mov.l @(4, %0), r9\n\t"
		 "mov.l @(8, %0), r10\n\t"
		 "mov.l @(12, %0), r11\n\t"
		 "mov.l @(16, %0), r12\n\t"
		 "mov.l @(20, %0), r13\n\t"
		 "mov.l @(24, %0), r14\n\t"
		 "mov.l @(28, %0), r15\n\t"
		 "mov.l @(32, %0), r0\n\t"
		 "ldc r0, sr\n\t"
		 "mov.l @(36, %0), r0\n\t"
		 "ldc r0, gbr\n\t"
		 "mov.l @(40, %0), r0\n\t"
		 "lds r0, pr\n\t"
		 "mov.l r0, @(0, r15)" :: "r" (addr));
}

/*--------------------------------------------------------------------------- 
 * Switch thread in round robin fashion.
 *---------------------------------------------------------------------------
 */
void
switch_thread(void)
{
  int		ct;
  int		nt;
  thread_t*	t = &threads;

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
