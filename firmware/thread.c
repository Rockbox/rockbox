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
    void*	lr;
    u_int32_t	cr;
    void*	sp;
  } regs;
  u_int32_t	mem[32];
} ctx_t;

typedef struct
{
  int		created;
  int		current;
  ctx_t		ctx[MAXTHREADS] __attribute__ ((aligned (32)));
} thread_t;

static thread_t threads = {1, 0};

/*--------------------------------------------------------------------------- 
 * Store non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void
stctx(void*	addr)
{
  u_int32_t	tmp;

  __asm__ __volatile__ ("mflr %0" : "=r" (tmp));
  __asm__ __volatile__ ("stw  %0,0(%1)" :: "r" (tmp), "b" (addr) : "memory");
  __asm__ __volatile__ ("mfcr %0" : "=r" (tmp));
  __asm__ __volatile__ ("stw  %0,4(%1)" :: "r" (tmp), "b" (addr) : "memory");
  __asm__ __volatile__ ("stw 1, 8(%0)\n\t"
			"stw 2, 12(%0)\n\t"
			"stw 13,16(%0)\n\t"
			"stw 14,20(%0)\n\t"
			"stw 15,24(%0)\n\t"
			"stw 16,28(%0)\n\t"
			"stw 17,32(%0)\n\t"
			"stw 18,36(%0)\n\t"
			"stw 19,40(%0)\n\t"
			"stw 20,44(%0)\n\t"
			"stw 21,48(%0)\n\t"
			"stw 22,52(%0)\n\t"
			"stw 23,56(%0)\n\t"
			"stw 24,60(%0)\n\t"
			"stw 25,64(%0)\n\t"
			"stw 26,68(%0)\n\t"
			"stw 27,72(%0)\n\t"
			"stw 28,76(%0)\n\t"
			"stw 29,80(%0)\n\t"
			"stw 30,84(%0)\n\t"
			"stw 31,88(%0)\n\t"
			:: "b" (addr) : "memory");
}

/*--------------------------------------------------------------------------- 
 * Load non-volatile context.
 *---------------------------------------------------------------------------
 */
static inline void
ldctx(void*	addr)
{
  u_int32_t	tmp;

  __asm__ __volatile__ ("lwz  %0,0(%1)" : "=r" (tmp): "b" (addr) : "memory");
  __asm__ __volatile__ ("mtlr %0" : "=r" (tmp));
  __asm__ __volatile__ ("lwz  %0,4(%1)" : "=r" (tmp): "b" (addr) : "memory");
  __asm__ __volatile__ ("mtcr %0" : "=r" (tmp));
  __asm__ __volatile__ ("lwz 1, 8(%0)\n\t"
			"lwz 2, 12(%0)\n\t"
			"lwz 13,16(%0)\n\t"
			"lwz 14,20(%0)\n\t"
			"lwz 15,24(%0)\n\t"
			"lwz 16,28(%0)\n\t"
			"lwz 17,32(%0)\n\t"
			"lwz 18,36(%0)\n\t"
			"lwz 19,40(%0)\n\t"
			"lwz 20,44(%0)\n\t"
			"lwz 21,48(%0)\n\t"
			"lwz 22,52(%0)\n\t"
			"lwz 23,56(%0)\n\t"
			"lwz 24,60(%0)\n\t"
			"lwz 25,64(%0)\n\t"
			"lwz 26,68(%0)\n\t"
			"lwz 27,72(%0)\n\t"
			"lwz 28,76(%0)\n\t"
			"lwz 29,80(%0)\n\t"
			"lwz 30,84(%0)\n\t"
			"lwz 31,88(%0)\n\t"
			:: "b" (addr) : "memory");
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
int
create_thread(void*	fp,
	      void*	sp,
	      int	stk_size)
{
  thread_t*		t = &threads;

  if (t->created >= MAXTHREADS)
    return -1;
  else
    {
      ctx_t* ctxp = &t->ctx[t->created++];
      stctx(ctxp);
      ctxp->regs.sp = (void*)(((u_int32_t)sp + stk_size - 256) & ~31);
      ctxp->regs.lr = fp;
    }
  return 0;
}

/* eof */
