/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Daniel Ankers
 *
 * PP5002 and PP502x SoC threading support
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

#if NUM_CORES == 1
/* Single-core variants for FORCE_SINGLE_CORE */
static inline void core_sleep(void)
{
    sleep_core(CURRENT_CORE);
    enable_irq();
}

/* Shared single-core build debugging version */
void core_wake(void)
{
    /* No wakey - core already wakey (because this is it) */
}
#else /* NUM_CORES > 1 */
/** Model-generic PP dual-core code **/
extern uintptr_t cpu_idlestackbegin[];
extern uintptr_t cpu_idlestackend[];
extern uintptr_t cop_idlestackbegin[];
extern uintptr_t cop_idlestackend[];
uintptr_t * const idle_stacks[NUM_CORES] =
{
    [CPU] = cpu_idlestackbegin,
    [COP] = cop_idlestackbegin
};


/*---------------------------------------------------------------------------
 * Do any device-specific inits for the threads and synchronize the kernel
 * initializations.
 *---------------------------------------------------------------------------
 */
static void INIT_ATTR core_thread_init(unsigned int core) 
{
    if (core == CPU)
    {
        /* Wake up coprocessor and let it initialize kernel and threads */
#ifdef CPU_PP502x
        MBX_MSG_CLR = 0x3f;
#endif
        wake_core(COP);
        /* Sleep until COP has finished */
        sleep_core(CPU);
    }
    else
    {
        /* Wake the CPU and return */
        wake_core(CPU);
    }
}

/*---------------------------------------------------------------------------
 * Switches to a stack that always resides in the Rockbox core then calls
 * the final exit routine to actually finish removing the thread from the
 * scheduler.
 *
 * Needed when a thread suicides on a core other than the main CPU since the
 * stack used when idling is the stack of the last thread to run. This stack
 * may not reside in the core firmware in which case the core will continue
 * to use a stack from an unloaded module until another thread runs on it.
 *---------------------------------------------------------------------------
 */
static void __attribute__((naked, noinline, noreturn))
    thread_exit_finalize(unsigned int core, struct thread_entry *current)
{
    asm volatile (
        "ldr    r2, =idle_stacks     \n" /* switch to idle stack  */
        "ldr    sp, [r2, r0, lsl #2] \n"
        "add    sp, sp, %0*4         \n"
        "cmp    r0, #0               \n" /* CPU? */
        "mov    r4, r1               \n"
        "blne   commit_dcache        \n"
        "mov    r0, r4               \n"
        "b      thread_exit_final    \n"
        : : "i"(IDLE_STACK_WORDS));

    while (1);
    (void)core; (void)current;
}

/*---------------------------------------------------------------------------
 * Machine-specific helper function for switching the processor a thread is
 * running on. Basically, the thread suicides on the departing core and is
 * reborn on the destination. Were it not for gcc's ill-behavior regarding
 * naked functions written in C where it actually clobbers non-volatile
 * registers before the intended prologue code, this would all be much
 * simpler.  Generic setup is done in switch_core itself.
 */

/*---------------------------------------------------------------------------
 * This actually performs the core switch.
 */
static void __attribute__((naked, noinline))
    switch_thread_core(unsigned int old_core, struct thread_entry *thread)
{
    asm volatile (
        "stmfd  sp!, { r4-r5, lr }     \n" /* can't use the first two ctx fields */
        "add    r2, r1, #8             \n"
        "stmia  r2, { r6-r11, sp }     \n" /* save remaining context */
        "adr    r2, .new_core_restart  \n" /* save context ptr + restart address */
        "str    r2, [r1, #40]          \n" /* make 'start' non-null */
        "stmia  r1, { r1-r2 }          \n"
        "ldr    r2, =idle_stacks       \n" /* switch to idle stack on old core */
        "ldr    sp, [r2, r0, lsl #2]   \n"
        "add    sp, sp, %0*4           \n"
        "stmfd  sp!, { r0-r1 }         \n"
        "bl     commit_dcache          \n" /* write back everything */
        "ldmfd  sp!, { r0-r1 }         \n"
        "b      switch_core_final      \n"
    ".new_core_restart:                \n"
        "mov    r1, #0                 \n" /* mark as started */
        "str    r1, [r0, #40]          \n"
        "add    r0, r0, #8             \n"
        "ldmia  r0, { r6-r11, sp }     \n" /* restore non-volatiles and stack */
        "bl     commit_discard_idcache \n" /* invalidate new core's cache */
        "ldmfd  sp!, { r4-r5, pc }     \n" /* restore remaining context */
        : : "i"(IDLE_STACK_WORDS));
    (void)old_core; (void)thread;
}

/** PP-model-specific dual-core code **/

#if CONFIG_CPU == PP5002
/* PP5002 has no mailboxes - Bytes to emulate the PP502x mailbox bits */
struct core_semaphores
{
    volatile uint8_t intend_wake;  /* 00h */
    volatile uint8_t stay_awake;   /* 01h */
    volatile uint8_t intend_sleep; /* 02h */
    volatile uint8_t unused;       /* 03h */
};

static struct core_semaphores core_semaphores[NUM_CORES] IBSS_ATTR;

#if 1 /* Select ASM */
/*---------------------------------------------------------------------------
 * Put core in a power-saving state if waking list wasn't repopulated and if
 * no other core requested a wakeup for it to perform a task.
 *---------------------------------------------------------------------------
 */
static inline void core_sleep(unsigned int core)
{
    asm volatile (
        "mov    r0, #1                     \n" /* Signal intent to sleep */
        "strb   r0, [%[sem], #2]           \n"
        "ldrb   r0, [%[sem], #1]           \n" /* && stay_awake == 0? */
        "cmp    r0, #0                     \n"
        "bne    2f                         \n"
        /* Sleep: PP5002 crashes if the instruction that puts it to sleep is
         * located at 0xNNNNNNN0. 4/8/C works. This sequence makes sure
         * that the correct alternative is executed. Don't change the order
         * of the next 4 instructions! */
        "tst    pc, #0x0c                  \n"
        "mov    r0, #0xca                  \n"
        "strne  r0, [%[ctl], %[c], lsl #2] \n"
        "streq  r0, [%[ctl], %[c], lsl #2] \n"
        "nop                               \n" /* nop's needed because of pipeline */
        "nop                               \n"
        "nop                               \n"
    "2:                                    \n"
        "mov    r0, #0                     \n" /* Clear stay_awake and sleep intent */
        "strb   r0, [%[sem], #1]           \n"
        "strb   r0, [%[sem], #2]           \n"
    "1:                                    \n" /* Wait for wake procedure to finish */
        "ldrb   r0, [%[sem], #0]           \n"
        "cmp    r0, #0                     \n"
        "bne    1b                         \n"
        :
        : [sem]"r"(&core_semaphores[core]), [c]"r"(core),
          [ctl]"r"(&CPU_CTL)
        : "r0"
        );
    enable_irq();
}

/*---------------------------------------------------------------------------
 * Wake another processor core that is sleeping or prevent it from doing so
 * if it was already destined. FIQ, IRQ should be disabled before calling.
 *---------------------------------------------------------------------------
 */
void core_wake(unsigned int othercore)
{
    /* avoid r0 since that contains othercore */
    asm volatile (
        "mrs    r3, cpsr                \n" /* Disable IRQ */
        "orr    r1, r3, #0x80           \n"
        "msr    cpsr_c, r1              \n"
        "mov    r1, #1                  \n" /* Signal intent to wake other core */
        "orr    r1, r1, r1, lsl #8      \n" /* and set stay_awake */
        "strh   r1, [%[sem], #0]        \n"
        "mov    r2, #0x8000             \n"
    "1:                                 \n" /* If it intends to sleep, let it first */
        "ldrb   r1, [%[sem], #2]        \n" /* intend_sleep != 0 ? */
        "cmp    r1, #1                  \n"
        "ldr    r1, [%[st]]             \n" /* && not sleeping ? */
        "tsteq  r1, r2, lsr %[oc]       \n"
        "beq    1b                      \n" /* Wait for sleep or wake */
        "tst    r1, r2, lsr %[oc]       \n"
        "ldrne  r2, =0xcf004054         \n" /* If sleeping, wake it */
        "movne  r1, #0xce               \n"
        "strne  r1, [r2, %[oc], lsl #2] \n"
        "mov    r1, #0                  \n" /* Done with wake procedure */
        "strb   r1, [%[sem], #0]        \n"
        "msr    cpsr_c, r3              \n" /* Restore IRQ */
        :
        : [sem]"r"(&core_semaphores[othercore]),
          [st]"r"(&PROC_STAT),
          [oc]"r"(othercore)
        : "r1", "r2", "r3"
    );
}

#else /* C version for reference */

static inline void core_sleep(unsigned int core)
{
    /* Signal intent to sleep */
    core_semaphores[core].intend_sleep = 1;

    /* Something waking or other processor intends to wake us? */
    if (core_semaphores[core].stay_awake == 0)
    {
        sleep_core(core);
    }

    /* Signal wake - clear wake flag */
    core_semaphores[core].stay_awake = 0;
    core_semaphores[core].intend_sleep = 0;

    /* Wait for other processor to finish wake procedure */
    while (core_semaphores[core].intend_wake != 0);

    /* Enable IRQ */
    enable_irq();
}

void core_wake(unsigned int othercore)
{
    /* Disable interrupts - avoid reentrancy from the tick */
    int oldlevel = disable_irq_save();

    /* Signal intent to wake other processor - set stay awake */
    core_semaphores[othercore].intend_wake = 1;
    core_semaphores[othercore].stay_awake = 1;

    /* If it intends to sleep, wait until it does or aborts */
    while (core_semaphores[othercore].intend_sleep != 0 &&
           (PROC_STAT & PROC_SLEEPING(othercore)) == 0);

    /* If sleeping, wake it up */
    if (PROC_STAT & PROC_SLEEPING(othercore))
        wake_core(othercore);

    /* Done with wake procedure */
    core_semaphores[othercore].intend_wake = 0;
    restore_irq(oldlevel);
}
#endif  /* ASM/C selection */

#elif defined (CPU_PP502x)

#if 1 /* Select ASM */
/*---------------------------------------------------------------------------
 * Put core in a power-saving state if waking list wasn't repopulated and if
 * no other core requested a wakeup for it to perform a task.
 *---------------------------------------------------------------------------
 */
static inline void core_sleep(unsigned int core)
{
    asm volatile (
        "mov    r0, #4                     \n" /* r0 = 0x4 << core */
        "mov    r0, r0, lsl %[c]           \n"
        "str    r0, [%[mbx], #4]           \n" /* signal intent to sleep */
        "ldr    r1, [%[mbx], #0]           \n" /* && !(MBX_MSG_STAT & (0x10<<core)) ? */
        "tst    r1, r0, lsl #2             \n"  
        "moveq  r1, #0x80000000            \n" /* Then sleep */
        "streq  r1, [%[ctl], %[c], lsl #2] \n"
        "moveq  r1, #0                     \n" /* Clear control reg */
        "streq  r1, [%[ctl], %[c], lsl #2] \n"
        "orr    r1, r0, r0, lsl #2         \n" /* Signal intent to wake - clear wake flag */
        "str    r1, [%[mbx], #8]           \n"
    "1:                                    \n" /* Wait for wake procedure to finish */
        "ldr    r1, [%[mbx], #0]           \n"
        "tst    r1, r0, lsr #2             \n"
        "bne    1b                         \n"
        :
        :  [ctl]"r"(&CPU_CTL), [mbx]"r"(MBX_BASE), [c]"r"(core)
        : "r0", "r1");
    enable_irq();
}

/*---------------------------------------------------------------------------
 * Wake another processor core that is sleeping or prevent it from doing so
 * if it was already destined. FIQ, IRQ should be disabled before calling.
 *---------------------------------------------------------------------------
 */
void core_wake(unsigned int othercore)
{
    /* avoid r0 since that contains othercore */
    asm volatile (
        "mrs    r3, cpsr                    \n" /* Disable IRQ */
        "orr    r1, r3, #0x80               \n"
        "msr    cpsr_c, r1                  \n"
        "mov    r2, #0x11                   \n" /* r2 = (0x11 << othercore) */
        "mov    r2, r2, lsl %[oc]           \n" /* Signal intent to wake othercore */
        "str    r2, [%[mbx], #4]            \n"
    "1:                                     \n" /* If it intends to sleep, let it first */
        "ldr    r1, [%[mbx], #0]            \n" /* (MSG_MSG_STAT & (0x4 << othercore)) != 0 ? */
        "eor    r1, r1, #0xc                \n"
        "tst    r1, r2, lsr #2              \n"
        "ldr    r1, [%[ctl], %[oc], lsl #2] \n" /* && (PROC_CTL(othercore) & PROC_SLEEP) == 0 ? */
        "tsteq  r1, #0x80000000             \n"
        "beq    1b                          \n" /* Wait for sleep or wake */
        "tst    r1, #0x80000000             \n" /* If sleeping, wake it */
        "movne  r1, #0x0                    \n"
        "strne  r1, [%[ctl], %[oc], lsl #2] \n"
        "mov    r1, r2, lsr #4              \n"
        "str    r1, [%[mbx], #8]            \n" /* Done with wake procedure */
        "msr    cpsr_c, r3                  \n" /* Restore IRQ */
        :
        : [ctl]"r"(&PROC_CTL(CPU)), [mbx]"r"(MBX_BASE),
          [oc]"r"(othercore)
        : "r1", "r2", "r3");
}

#else /* C version for reference */

static inline void core_sleep(unsigned int core)
{
    /* Signal intent to sleep */
    MBX_MSG_SET = 0x4 << core;

    /* Something waking or other processor intends to wake us? */
    if ((MBX_MSG_STAT & (0x10 << core)) == 0)
    {
        sleep_core(core);
        wake_core(core);
    }

    /* Signal wake - clear wake flag */
    MBX_MSG_CLR = 0x14 << core;

    /* Wait for other processor to finish wake procedure */
    while (MBX_MSG_STAT & (0x1 << core));
    enable_irq();
}

void core_wake(unsigned int othercore)
{
    /* Disable interrupts - avoid reentrancy from the tick */
    int oldlevel = disable_irq_save();

    /* Signal intent to wake other processor - set stay awake */
    MBX_MSG_SET = 0x11 << othercore;

    /* If it intends to sleep, wait until it does or aborts */
    while ((MBX_MSG_STAT & (0x4 << othercore)) != 0 &&
           (PROC_CTL(othercore) & PROC_SLEEP) == 0);

    /* If sleeping, wake it up */
    if (PROC_CTL(othercore) & PROC_SLEEP)
        PROC_CTL(othercore) = 0;

    /* Done with wake procedure */
    MBX_MSG_CLR = 0x1 << othercore;
    restore_irq(oldlevel);
}
#endif /* ASM/C selection */

#endif /* CPU_PPxxxx */

/* Keep constant pool in range of inline ASM */
static void __attribute__((naked)) USED_ATTR dump_ltorg(void)
{
    asm volatile (".ltorg");
}

#endif /* NUM_CORES */
