#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "mmu-imx31.h"

#include "lcd.h"

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
}

void system_reboot(void)
{
}

void system_init(void)
{
}


#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
    (void)freqency;
}

#endif
