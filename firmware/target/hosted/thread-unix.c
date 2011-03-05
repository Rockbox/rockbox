/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Thomas Martitz
 *
 * Generic unix threading support
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <stdio.h>
#include <setjmp.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include "debug.h"

static volatile bool sig_handler_called;
static volatile jmp_buf tramp_buf;
static volatile jmp_buf bootstrap_buf;
static void (*thread_func)(void);
static const int trampoline_sig = SIGUSR1;
static pthread_t main_thread;

static struct ctx {
    jmp_buf thread_buf;
} thread_bufs[MAXTHREADS];
static struct ctx* thread_context, *target_context;
static int curr_uc;

static void trampoline(int sig);
static void bootstrap_context(void) __attribute__((noinline));

/* The *_context functions are heavily based on Gnu pth
 * http://www.gnu.org/software/pth/
 *
 * adjusted to work in a multi-thread environment to
 * offer a ucontext-like API
 */

/*
 * VARIANT 2: THE SIGNAL STACK TRICK
 *
 * This uses sigstack/sigaltstack() and friends and is really the
 * most tricky part of Pth. When you understand the following
 * stuff you're a good Unix hacker and then you've already
 * understood the gory ingredients of Pth.  So, either welcome to
 * the club of hackers, or do yourself a favor and skip this ;)
 *
 * The ingenious fact is that this variant runs really on _all_ POSIX
 * compliant systems without special platform kludges.  But be _VERY_
 * carefully when you change something in the following code. The slightest
 * change or reordering can lead to horribly broken code.  Really every
 * function call in the following case is intended to be how it is, doubt
 * me...
 *
 * For more details we strongly recommend you to read the companion
 * paper ``Portable Multithreading -- The Signal Stack Trick for
 * User-Space Thread Creation'' from Ralf S. Engelschall. A copy of the
 * draft of this paper you can find in the file rse-pmt.ps inside the
 * GNU Pth distribution.
 */

static int make_context(struct ctx *ctx, void (*f)(void), char *sp, size_t stack_size)
{
    struct sigaction sa;
    struct sigaction osa;
    stack_t ss;
    stack_t oss;
    sigset_t osigs;
    sigset_t sigs;

    disable_irq();
    /*
     * Preserve the trampoline_sig signal state, block trampoline_sig,
     * and establish our signal handler. The signal will
     * later transfer control onto the signal stack.
     */
    sigemptyset(&sigs);
    sigaddset(&sigs, trampoline_sig);
    sigprocmask(SIG_BLOCK, &sigs, &osigs);
    sa.sa_handler = trampoline;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK;
    if (sigaction(trampoline_sig, &sa, &osa) != 0)
    {
        DEBUGF("%s(): %s\n", __func__, strerror(errno));
        return false;
    }
    /*
     * Set the new stack.
     *
     * For sigaltstack we're lucky [from sigaltstack(2) on
     * FreeBSD 3.1]: ``Signal stacks are automatically adjusted
     * for the direction of stack growth and alignment
     * requirements''
     *
     * For sigstack we have to decide ourself [from sigstack(2)
     * on Solaris 2.6]: ``The direction of stack growth is not
     * indicated in the historical definition of struct sigstack.
     * The only way to portably establish a stack pointer is for
     * the application to determine stack growth direction.''
     */
    ss.ss_sp    = sp;
    ss.ss_size  = stack_size;
    ss.ss_flags = 0;
    if (sigaltstack(&ss, &oss) < 0)
    {
        DEBUGF("%s(): %s\n", __func__, strerror(errno));
        return false;
    }

    /*
     * Now transfer control onto the signal stack and set it up.
     * It will return immediately via "return" after the setjmp()
     * was performed. Be careful here with race conditions.  The
     * signal can be delivered the first time sigsuspend() is
     * called.
     */
    sig_handler_called = false;
    main_thread = pthread_self();
    sigfillset(&sigs);
    sigdelset(&sigs, trampoline_sig);
    pthread_kill(main_thread, trampoline_sig);
    while(!sig_handler_called)
        sigsuspend(&sigs);

    /*
     * Inform the system that we are back off the signal stack by
     * removing the alternative signal stack. Be careful here: It
     * first has to be disabled, before it can be removed.
     */
    sigaltstack(NULL, &ss);
    ss.ss_flags = SS_DISABLE;
    if (sigaltstack(&ss, NULL) < 0)
    {
        DEBUGF("%s(): %s\n", __func__, strerror(errno));
        return false;
    }
    sigaltstack(NULL, &ss);
    if (!(ss.ss_flags & SS_DISABLE))
    {
        DEBUGF("%s(): %s\n", __func__, strerror(errno));
        return false;
    }
    if (!(oss.ss_flags & SS_DISABLE))
        sigaltstack(&oss, NULL);

    /*
     * Restore the old trampoline_sig signal handler and mask
     */
    sigaction(trampoline_sig, &osa, NULL);
    sigprocmask(SIG_SETMASK, &osigs, NULL);

    /*
     * Tell the trampoline and bootstrap function where to dump
     * the new machine context, and what to do afterwards...
     */
    thread_func = f;
    thread_context  = ctx;

    /*
     * Now enter the trampoline again, but this time not as a signal
     * handler. Instead we jump into it directly. The functionally
     * redundant ping-pong pointer arithmentic is neccessary to avoid
     * type-conversion warnings related to the `volatile' qualifier and
     * the fact that `jmp_buf' usually is an array type.
     */
    if (setjmp(*((jmp_buf *)&bootstrap_buf)) == 0)
        longjmp(*((jmp_buf *)&tramp_buf), 1);

    /*
     * Ok, we returned again, so now we're finished
     */
    enable_irq();
    return true;
}

static void trampoline(int sig)
{
    (void)sig;
    /* sanity check, no other thread should be here */
    if (pthread_self() != main_thread)
        return;

    if (setjmp(*((jmp_buf *)&tramp_buf)) == 0)
    {
        sig_handler_called = true;
        return;
    }
    /* longjump'd back in */
    bootstrap_context();
}

void bootstrap_context(void)
{
    /* copy to local storage so we can spawn further threads
     * in the meantime */
    void (*thread_entry)(void) = thread_func;
    struct ctx *t = thread_context;
    
    /*
     * Save current machine state (on new stack) and
     * go back to caller until we're scheduled for real...
     */
    if (setjmp(t->thread_buf) == 0)
        longjmp(*((jmp_buf *)&bootstrap_buf), 1);

    /*
     * The new thread is now running: GREAT!
     * Now we just invoke its init function....
     */
    thread_entry();
    DEBUGF("thread left\n");
    thread_exit();
}

static inline void set_context(struct ctx *c)
{
    longjmp(c->thread_buf, 1);
}

static inline void swap_context(struct ctx *old, struct ctx *new)
{
    if (setjmp(old->thread_buf) == 0)
        longjmp(new->thread_buf, 1);
}

static inline void get_context(struct ctx *c)
{
    setjmp(c->thread_buf);
}


static void setup_thread(struct regs *context);

#define INIT_MAIN_THREAD
static void init_main_thread(void *addr)
{
    /* get a context for the main thread so that we can jump to it from
     * other threads */
    struct regs *context = (struct regs*)addr;
    context->uc = &thread_bufs[curr_uc++];
    get_context(context->uc);
}

#define THREAD_STARTUP_INIT(core, thread, function) \
    ({ (thread)->context.stack_size = (thread)->stack_size, \
       (thread)->context.stack = (uintptr_t)(thread)->stack; \
       (thread)->context.start = function; })



/*
 * Prepare context to make the thread runnable by calling swapcontext on it
 */
static void setup_thread(struct regs *context)
{
    void (*fn)(void) = context->start;
    context->uc = &thread_bufs[curr_uc++];
    while (!make_context(context->uc, fn, (char*)context->stack, context->stack_size))
        DEBUGF("Thread creation failed. Retrying");
}


/*
 * Save the ucontext_t pointer for later use in swapcontext()
 *
 * Cannot do getcontext() here, because jumping back to the context
 * resumes after the getcontext call (i.e. store_context), but we need
 * to resume from load_context()
 */
static inline void store_context(void* addr)
{
    struct regs *r = (struct regs*)addr;
    target_context = r->uc;
}

/*
 * Perform context switch
 */
static inline void load_context(const void* addr)
{
    struct regs *r = (struct regs*)addr;
    if (UNLIKELY(r->start))
    {
        setup_thread(r);
        r->start = NULL;
    }
    swap_context(target_context, r->uc);
}

/*
 * play nice with the host and sleep while waiting for the tick */
extern void wait_for_interrupt(void);
static inline void core_sleep(void)
{
    enable_irq();
    wait_for_interrupt();
}
