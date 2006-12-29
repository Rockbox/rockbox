#include "kernel.h"
#include "system.h"
#include "panic.h"

#include "lcd.h"
#include <stdio.h>

const int TIMER4_MASK = 1 << 14;

int system_memory_guard(int newmode) 
{
    (void)newmode;
    return 0;
}

extern void timer4(void);

void irq(void) 
{
    int intpending = INTPND;

    SRCPND = intpending; /* Clear this interrupt. */
    INTPND = intpending; /* Clear this interrupt. */

    /* Timer 4 */
    if ((intpending & TIMER4_MASK) != 0)
    {
        timer4();
    }
    else 
    {
        /* unexpected interrupt */
    }
}

