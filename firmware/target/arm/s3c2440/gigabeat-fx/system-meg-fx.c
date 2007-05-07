#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "mmu-meg-fx.h"

#include "lcd.h"

enum
{
    TIMER4_MASK = (1 << 14),
    LCD_MASK   =  (1 << 16),
    DMA0_MASK   = (1 << 17),
    DMA1_MASK   = (1 << 18),
    DMA2_MASK   = (1 << 19),
    DMA3_MASK   = (1 << 20),
    ALARM_MASK  = (1 << 30),
};

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

extern void timer4(void);
extern void dma0(void); /* free */
extern void dma1(void);
extern void dma3(void);

void irq(void)
{
    int intpending = INTPND;

    SRCPND = intpending; /* Clear this interrupt. */
    INTPND = intpending; /* Clear this interrupt. */

    /* Timer 4 */
    if ((intpending & TIMER4_MASK) != 0)
        timer4();
    else
    {
        /* unexpected interrupt */
    }
}

void system_reboot(void)
{
    WTCON = 0;
    WTCNT = WTDAT = 1 ;
    WTCON = 0x21;
    for(;;)
        ;
}

void system_init(void)
{
    /* Turn off un-needed devices */

    /* Turn off all of the UARTS */
    CLKCON &= ~( (1<<10) | (1<<11) |(1<<12) );

    /* Turn off AC97 and Camera */
    CLKCON &= ~( (1<<19) | (1<<20) );

    /* Turn off USB host */
    CLKCON &= ~(1 << 6);
    
    /* Turn off USB device */
    CLKCON &= ~(1 << 7);

    /* Turn off NAND flash controller */
    CLKCON &= ~(1 << 4);
    
    /* Turn off the USB PLL */
    CLKSLOW |= (1 << 7);

}


#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
    if (frequency == CPUFREQ_MAX)
    {
        asm volatile("mov r0, #0\n"
            "mrc p15, 0, r0, c1, c0, 0\n"
            "orr r0, r0, #3<<30\n" /* set to Asynchronous mode*/
            "mcr p15, 0, r0, c1, c0, 0" : : : "r0");

        FREQ = CPUFREQ_MAX;
    }
    else
    {
        asm volatile("mov r0, #0\n"
            "mrc p15, 0, r0, c1, c0, 0\n"
            "bic r0, r0, #3<<30\n" /* set to FastBus mode*/
            "mcr p15, 0, r0, c1, c0, 0" : : : "r0");

        FREQ = CPUFREQ_NORMAL;
    }
}

#endif
