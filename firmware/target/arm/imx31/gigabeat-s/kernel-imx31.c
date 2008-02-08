#include "config.h"
#include "system.h"
#include "avic-imx31.h"
#include "kernel.h"
#include "thread.h"
#include <stdio.h>

extern void (*tick_funcs[MAX_NUM_TICK_TASKS])(void);

static __attribute__((interrupt("IRQ"))) void EPIT1_HANDLER(void)
{
    int i;

    EPITSR1 = 1; /* Clear the pending status */

    /* Run through the list of tick tasks */
    for(i = 0;i < MAX_NUM_TICK_TASKS;i++)
    {
        if(tick_funcs[i])
            tick_funcs[i]();
    }

    current_tick++;
}

void tick_start(unsigned int interval_in_ms)
{
    CLKCTL_CGR0 |= (3 << 6);   /* EPIT1 module clock ON - before writing regs! */
    EPITCR1 &= ~((1 << 2) | (1 << 0)); /* Disable the counter */
    CLKCTL_WIMR0 &= ~(1 << 23);  /* Clear wakeup mask */

    /* NOTE: This isn't really accurate yet but it's close enough to work
     * with for the moment */

    /* CLKSRC=32KHz, EPIT Output Disconnected, Enabled
     * prescale 1/32, Reload from modulus register, Compare interrupt enabled,
     * Count from load value */
    EPITCR1 = (3 << 24) | (1 << 19) | (32 << 4) |
              (1 << 3) | (1 << 2) | (1 << 1);

    EPITLR1 = interval_in_ms;   /* Count down from interval */
    EPITCMPR1 = 0;              /* Event when counter reaches 0 */
    EPITSR1 = 1;                /* Clear any pending interrupt */
    avic_enable_int(EPIT1, IRQ, 7, EPIT1_HANDLER);
    EPITCR1 |= (1 << 0);        /* Enable the counter */
}

#ifdef BOOTLOADER
void tick_stop(void)
{
    avic_disable_int(EPIT1);    /* Disable insterrupt */
    EPITCR1 &= ~((1 << 2) | (1 << 0)); /* Disable counter */
    CLKCTL_CGR0 &= ~(3 << 6);   /* EPIT1 module clock OFF */
}
#endif
