/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include "config.h"
#include <stdbool.h>
#include "lcd.h"
#include "font.h"
#include "system.h"
#include "kernel.h"
#include "timer.h"

#ifndef SIMULATOR
long cpu_frequency = CPU_FREQ;
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
int boost_counter = 0;
bool cpu_idle = false;
void cpu_boost(bool on_off)
{
    if(on_off)
    {
        /* Boost the frequency if not already boosted */
        if(boost_counter++ == 0)
        {
            set_cpu_frequency(CPUFREQ_MAX);
        }
    }
    else
    {
        /* Lower the frequency if the counter reaches 0 */
        if(--boost_counter == 0)
        {
            if(cpu_idle)
                set_cpu_frequency(CPUFREQ_DEFAULT);
            else
                set_cpu_frequency(CPUFREQ_NORMAL);
        }

        /* Safety measure */
        if(boost_counter < 0)
            boost_counter = 0;
    }
}

void cpu_idle_mode(bool on_off)
{
    cpu_idle = on_off;

    /* We need to adjust the frequency immediately if the CPU
       isn't boosted */
    if(boost_counter == 0)
    {
        if(cpu_idle)
            set_cpu_frequency(CPUFREQ_DEFAULT);
        else
            set_cpu_frequency(CPUFREQ_NORMAL);
    }
}

#endif

#if CONFIG_CPU == TCC730

void* volatile interrupt_vector[16]  __attribute__ ((section(".idata")));

static void ddma_wait_idle(void)  __attribute__ ((section (".icode")));
static void ddma_wait_idle(void)
{
    /* TODO: power saving trick: set the CPU freq to 22MHz
       while doing the busy wait after a disk dma access.
       (Used by Archos) */
    do {
    } while ((DDMACOM & 3) != 0);
}

void ddma_transfer(int dir, int mem, void* intAddr, long extAddr, int num)
    __attribute__ ((section (".icode")));
void ddma_transfer(int dir, int mem, void* intAddr, long extAddr, int num) {
    int irq = set_irq_level(1);
    ddma_wait_idle();
    long externalAddress = (long) extAddr;
    long internalAddress = ((long) intAddr) & 0xFFFF;
    /* HW wants those two in word units. */
    num /= 2;
    externalAddress /= 2;
    
    DDMACFG = (dir << 1) | (mem << 2);
    DDMAIADR = internalAddress;
    DDMAEADR = externalAddress;
    DDMANUM = num;
    DDMACOM |= 0x4; /* start */
    
    ddma_wait_idle(); /* wait for completion */
    set_irq_level(irq);
}

static void ddma_wait_idle_noicode(void)
{
    do {
    } while ((DDMACOM & 3) != 0);
}

static void ddma_transfer_noicode(int dir, int mem, long intAddr, long extAddr, int num) {
    int irq = set_irq_level(1);
    ddma_wait_idle_noicode();
    long externalAddress = (long) extAddr;
    long internalAddress = (long) intAddr;
    /* HW wants those two in word units. */
    num /= 2;
    externalAddress /= 2;
    
    DDMACFG = (dir << 1) | (mem << 2);
    DDMAIADR = internalAddress;
    DDMAEADR = externalAddress;
    DDMANUM = num;
    DDMACOM |= 0x4; /* start */
    
    ddma_wait_idle_noicode(); /* wait for completion */
    set_irq_level(irq);
}

/* Some linker-defined symbols */
extern int icodecopy;
extern int icodesize;
extern int icodestart;

/* change the a PLL frequency */
void set_pll_freq(int pll_index, long freq_out) {
    volatile unsigned int* plldata;
    volatile unsigned char* pllcon;
    if (pll_index == 0) {
        plldata = &PLL0DATA;
        pllcon = &PLL0CON;
    } else {
        plldata = &PLL1DATA;
        pllcon = &PLL1CON;
    }
    /* VC0 is 32768 Hz */
#define VC0FREQ (32768L)
    unsigned m = (freq_out / VC0FREQ) - 2;
    /* TODO: if m is too small here, use the divider bits [0,1] */
    *plldata = m << 2;
    *pllcon |= 0x1; /* activate */
    do {
    } while ((*pllcon & 0x2) == 0); /*  wait for stabilization */
}

int smsc_version(void) {
    int v;
    int* smsc_ver_addr = (int*)0x4C20;
    __asm__ ("ldc %0, @%1" : "=r"(v) : "a"(smsc_ver_addr));
    v &= 0xFF;
    if (v < 4 || v == 0xFF) {
        return 3;
    }
    return v;
}



void smsc_delay() {
    int i;
    /* FIXME: tune the delay.
       Delay doesn't depend on CPU speed in Archos' firmware.
    */
    for (i = 0; i < 100; i++) {
        
    }
}

static void extra_init(void) {
    /* Power on stuff */
    P1 |= 0x07;
    P1CON |= 0x1f;
    
    /* P5 conf
     * lines 0, 1 & 4 are digital, other analog. : 
     */
    P5CON = 0xec;

    P6CON = 0x19;

    /* P7 conf
       nothing to do: all are inputs
       (reset value of the register is good)
    */

    /* SMSC chip config (?) */
    P10CON |= 0x20;
    P6 &= 0xF7;
    P10 &= 0x20;
    smsc_delay();
    if (smsc_version() < 4) {
        P6 |= 0x08;
        P10 |= 0x20;
    }
}

void set_cpu_frequency(long frequency) {
    /* Enable SDRAM refresh, at least 15MHz */
    if (frequency < cpu_frequency)
        MIUDCNT = 0x800 | (frequency * 15/1000000L - 1);

    set_pll_freq(0, frequency);
    PLL0CON |= 0x4; /* use as CPU clock */

    cpu_frequency = frequency;
    /* wait states and such not changed by Archos. (!?) */

    /* Enable SDRAM refresh, 15MHz. */
    MIUDCNT = 0x800 | (frequency * 15/1000000L - 1);

    tick_start(1000/HZ);
    /* TODO: when uart is done; sync uart freq */
}

/* called by crt0 */
void system_init(void)
{
    /* Disable watchdog */
    WDTEN = 0xA5;

    /****************
     * GPIO ports
     */

    /* keep alive (?) -- clear the bit to prevent crash at start (??) */
    P8 = 0x00;
    P8CON = 0x01;

    /* smsc chip init (?) */
    P10 = 0x20;
    P6 = 0x08;

    P10CON = 0x20;
    P6CON = 0x08;

    /********
     * CPU
     */
    
    
    /* PLL0 (cpu osc. frequency) */
    /* set_cpu_frequency(CPU_FREQ); */


    /*******************
     * configure S(D)RAM 
     */

    /************************
     * Copy .icode section to icram
     */
    ddma_transfer_noicode(0, 0, 0x40, (long)&icodecopy, (int)&icodesize);
   

    /***************************
     * Interrupts
     */

    /* priorities ? */

    /* mask */
    IMR0 = 0;
    IMR1 = 0;


/*  IRQ0            BT INT */
/*  IRQ1           RTC INT */
/*  IRQ2            TA INT */
/*  IRQ3          TAOV INT */
/*  IRQ4            TB INT */
/*  IRQ5          TBOV INT */
/*  IRQ6            TC INT */
/*  IRQ7          TCOV INT */
/*  IRQ8           USB INT */
/*  IRQ9          PPIC INT */
/* IRQ10 UART_Rx/UART_Err/ UART_tx INT */
/* IRQ11            IIC INT */
/* IRQ12           SIO INT */
/* IRQ13           IIS0 INT */
/* IRQ14           IIS1 INT */
/* IRQ15               ­ */

    extra_init();
}

void system_reboot (void)
{
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
#elif defined(CPU_COLDFIRE)

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIE"))) void name (void)

static const char* const irqname[] = {
    "", "", "AccessErr","AddrErr","IllInstr", "", "","",
    "PrivVio","Trace","Line-A", "Line-F","Debug","","FormErr","Uninit",
    "","","","","","","","",
    "Spurious","Level1","Level2","Level3","Level4","Level5","Level6","Level7",
    "Trap0","Trap1","Trap2","Trap3","Trap4","Trap5","Trap6","Trap7",
    "Trap8","Trap9","Trap10","Trap11","Trap12","Trap13","Trap14","Trap15",
    "SWT","Timer0","Timer1","I2C","UART1","UART2","DMA0","DMA1",
    "DMA2","DMA3","QSPI","","","","","",
    "PDIR1FULL","PDIR2FULL","EBUTXEMPTY","IIS2TXEMPTY",
    "IIS1TXEMPTY","PDIR3FULL","PDIR3RESYN","UQ2CHANERR",
    "AUDIOTICK","PDIR2RESYN","PDIR2UNOV","PDIR1RESYN",
    "PDIR1UNOV","UQ1CHANERR","IEC2BUFATTEN","IEC2PARERR",
    "IEC2VALNOGOOD","IEC2CNEW","IEC1BUFATTEN","UCHANTXNF",
    "UCHANTXUNDER","UCHANTXEMPTY","PDIR3UNOV","IEC1PARERR",
    "IEC1VALNOGOOD","IEC1CNEW","EBUTXRESYN","EBUTXUNOV",
    "IIS2TXRESYN","IIS2TXUNOV","IIS1TXRESYN","IIS1TXUNOV",
    "GPIO0","GPI1","GPI2","GPI3","GPI4","GPI5","GPI6","GPI7",
    "","","","","","","","SOFTINT0",
    "SOFTINT1","SOFTINT2","SOFTINT3","",
    "","CDROMCRCERR","CDROMNOSYNC","CDROMILSYNC",
    "CDROMNEWBLK","","","","","","","",
    "","","","","","","","",
    "","","","","","","","",
    "","","","","","","","",
    "","","","","","","","",
    "","","","","","","","",
    "","","","","","","","",
    "","","","","","","","",
    "","","","","","","",""
};

default_interrupt (TRAP0); /* Trap #0 */
default_interrupt (TRAP1); /* Trap #1 */
default_interrupt (TRAP2); /* Trap #2 */
default_interrupt (TRAP3); /* Trap #3 */
default_interrupt (TRAP4); /* Trap #4 */
default_interrupt (TRAP5); /* Trap #5 */
default_interrupt (TRAP6); /* Trap #6 */
default_interrupt (TRAP7); /* Trap #7 */
default_interrupt (TRAP8); /* Trap #8 */
default_interrupt (TRAP9); /* Trap #9 */
default_interrupt (TRAP10); /* Trap #10 */
default_interrupt (TRAP11); /* Trap #11 */
default_interrupt (TRAP12); /* Trap #12 */
default_interrupt (TRAP13); /* Trap #13 */
default_interrupt (TRAP14); /* Trap #14 */
default_interrupt (TRAP15); /* Trap #15 */
default_interrupt (SWT); /* Software Watchdog Timer */
default_interrupt (TIMER0); /* Timer 0 */
default_interrupt (TIMER1); /* Timer 1 */
default_interrupt (I2C); /* I2C */
default_interrupt (UART1); /* UART 1 */
default_interrupt (UART2); /* UART 2 */
default_interrupt (DMA0); /* DMA 0 */
default_interrupt (DMA1); /* DMA 1 */
default_interrupt (DMA2); /* DMA 2 */
default_interrupt (DMA3); /* DMA 3 */
default_interrupt (QSPI); /* QSPI */

default_interrupt (PDIR1FULL); /* Processor data in 1 full */
default_interrupt (PDIR2FULL); /* Processor data in 2 full */
default_interrupt (EBUTXEMPTY); /* EBU transmit FIFO empty */
default_interrupt (IIS2TXEMPTY); /* IIS2 transmit FIFO empty */
default_interrupt (IIS1TXEMPTY); /* IIS1 transmit FIFO empty */
default_interrupt (PDIR3FULL); /* Processor data in 3 full */
default_interrupt (PDIR3RESYN); /* Processor data in 3 resync */
default_interrupt (UQ2CHANERR); /* IEC958-2 Rx U/Q channel error */
default_interrupt (AUDIOTICK); /* "tick" interrupt */
default_interrupt (PDIR2RESYN); /* Processor data in 2 resync */
default_interrupt (PDIR2UNOV); /* Processor data in 2 under/overrun */
default_interrupt (PDIR1RESYN); /* Processor data in 1 resync */
default_interrupt (PDIR1UNOV); /* Processor data in 1 under/overrun */
default_interrupt (UQ1CHANERR); /* IEC958-1 Rx U/Q channel error */
default_interrupt (IEC2BUFATTEN);/* IEC958-2 channel buffer full */
default_interrupt (IEC2PARERR); /* IEC958-2 Rx parity or symbol error */
default_interrupt (IEC2VALNOGOOD);/* IEC958-2 flag not good */
default_interrupt (IEC2CNEW); /* IEC958-2 New C-channel received */
default_interrupt (IEC1BUFATTEN);/* IEC958-1 channel buffer full */
default_interrupt (UCHANTXNF); /* U channel Tx reg next byte is first */
default_interrupt (UCHANTXUNDER);/* U channel Tx reg underrun */
default_interrupt (UCHANTXEMPTY);/* U channel Tx reg is empty */
default_interrupt (PDIR3UNOV); /* Processor data in 3 under/overrun */
default_interrupt (IEC1PARERR); /* IEC958-1 Rx parity or symbol error */
default_interrupt (IEC1VALNOGOOD);/* IEC958-1 flag not good */
default_interrupt (IEC1CNEW); /* IEC958-1 New C-channel received */
default_interrupt (EBUTXRESYN); /* EBU Tx FIFO resync */
default_interrupt (EBUTXUNOV); /* EBU Tx FIFO under/overrun */
default_interrupt (IIS2TXRESYN); /* IIS2 Tx FIFO resync */
default_interrupt (IIS2TXUNOV); /* IIS2 Tx FIFO under/overrun */
default_interrupt (IIS1TXRESYN); /* IIS1 Tx FIFO resync */
default_interrupt (IIS1TXUNOV); /* IIS1 Tx FIFO under/overrun */
default_interrupt (GPI0); /* GPIO interrupt 0 */
default_interrupt (GPI1); /* GPIO interrupt 1 */
default_interrupt (GPI2); /* GPIO interrupt 2 */
default_interrupt (GPI3); /* GPIO interrupt 3 */
default_interrupt (GPI4); /* GPIO interrupt 4 */
default_interrupt (GPI5); /* GPIO interrupt 5 */
default_interrupt (GPI6); /* GPIO interrupt 6 */
default_interrupt (GPI7); /* GPIO interrupt 7 */

default_interrupt (SOFTINT0); /* Software interrupt 0 */
default_interrupt (SOFTINT1); /* Software interrupt 1 */
default_interrupt (SOFTINT2); /* Software interrupt 2 */
default_interrupt (SOFTINT3); /* Software interrupt 3 */

default_interrupt (CDROMCRCERR); /* CD-ROM CRC error */
default_interrupt (CDROMNOSYNC); /* CD-ROM No sync */
default_interrupt (CDROMILSYNC); /* CD-ROM Illegal sync */
default_interrupt (CDROMNEWBLK); /* CD-ROM New block */

void UIE (void) /* Unexpected Interrupt or Exception */
{
    unsigned int format_vector, pc;
    int vector;
    char str[32];
    
    asm volatile ("move.l (52,%%sp),%0": "=r"(format_vector));
    asm volatile ("move.l (56,%%sp),%0": "=r"(pc));

    vector = (format_vector >> 18) & 0xff;
    
    /* clear screen */
    lcd_clear_display ();
#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);
#endif
    snprintf(str,sizeof(str),"I%02x:%s",vector,irqname[vector]);
    lcd_puts(0,0,str);
    snprintf(str,sizeof(str),"at %08x",pc);
    lcd_puts(0,1,str);
    lcd_update();
    
    /* set cpu frequency to 11mhz (to prevent overheating) */
    DCR = (DCR & ~0x01ff) | 1;
    PLLCR = 0x10800000;

    while (1)
    {
        /* check for the ON button (and !hold) */
        if ((GPIO1_READ & 0x22) == 0)
            SYPCR = 0xc0;
           /* Start watchdog timer with 512 cycles timeout. Don't service it. */

    /* We need a reset method that works in all cases. Calling system_reboot()
       doesn't work when we're called from the debug interrupt, because then
       the CPU is in emulator mode and the only ways leaving it are exexcuting
       an rte instruction or performing a reset. Even disabling the breakpoint
       logic and performing special rte magic doesn't make system_reboot()
       reliable. The system restarts, but boot often fails with ata error -42. */
    }
}

/* reset vectors are handled in crt0.S */
void (* const vbr[]) (void) __attribute__ ((section (".vectors"))) = 
{
    UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,TIMER0,TIMER1,UIE,UIE,UIE,
    /*          lvl 3  lvl 4             */

    TRAP0,TRAP1,TRAP2,TRAP3,TRAP4,TRAP5,TRAP6,TRAP7,
    TRAP8,TRAP9,TRAP10,TRAP11,TRAP12,TRAP13,TRAP14,TRAP15,
    
    SWT,UIE,UIE,I2C,UART1,UART2,DMA0,DMA1,
    DMA2,DMA3,QSPI,UIE,UIE,UIE,UIE,UIE,
    PDIR1FULL,PDIR2FULL,EBUTXEMPTY,IIS2TXEMPTY,
    IIS1TXEMPTY,PDIR3FULL,PDIR3RESYN,UQ2CHANERR,
    AUDIOTICK,PDIR2RESYN,PDIR2UNOV,PDIR1RESYN,
    PDIR1UNOV,UQ1CHANERR,IEC2BUFATTEN,IEC2PARERR,
    IEC2VALNOGOOD,IEC2CNEW,IEC1BUFATTEN,UCHANTXNF,
    UCHANTXUNDER,UCHANTXEMPTY,PDIR3UNOV,IEC1PARERR,
    IEC1VALNOGOOD,IEC1CNEW,EBUTXRESYN,EBUTXUNOV,
    IIS2TXRESYN,IIS2TXUNOV,IIS1TXRESYN,IIS1TXUNOV,
    GPI0,GPI1,GPI2,GPI3,GPI4,GPI5,GPI6,GPI7,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,SOFTINT0,
    SOFTINT1,SOFTINT2,SOFTINT3,UIE,
    UIE,CDROMCRCERR,CDROMNOSYNC,CDROMILSYNC,
    CDROMNEWBLK,UIE,UIE,UIE,UIE,UIE,UIE,UIE,

    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE
};

void system_init(void)
{
    /* Clear the accumulators. From here on it's the responsibility of
       whoever uses them to clear them after use (use movclr instruction). */
    asm volatile ("movclr.l %%acc0, %%d0\n\t"
                  "movclr.l %%acc1, %%d0\n\t"
                  "movclr.l %%acc2, %%d0\n\t"
                  "movclr.l %%acc3, %%d0\n\t"
                  : : : "d0");
}

void system_reboot (void)
{
    set_cpu_frequency(0);

    asm(" move.w #0x2700,%sr");
    /* Reset the cookie for the crt0 crash check */
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0,%sp");
    asm(" move.l 4,%a0");
    asm(" jmp (%a0)");
}

/* Utilise the breakpoint hardware to catch invalid memory accesses. */
int system_memory_guard(int newmode)
{
    static const unsigned long modes[MAXMEMGUARD][8] = {
        {   /* catch nothing */
            0x2C870000, 0x00000000, /* TDR  = 0x00000000 */
            0x2C8D0000, 0x00000000, /* ABLR = 0x00000000 */
            0x2C8C0000, 0x00000000, /* ABHR = 0x00000000 */
            0x2C860000, 0x00050000, /* AATR = 0x0005 */
        },
        {   /* catch flash ROM writes */
            0x2C8D0000, 0x00000000, /* ABLR = 0x00000000 */
            0x2C8C0FFF, 0xFFFF0000, /* ABHR = 0x0FFFFFFF */
            0x2C860000, 0x6F050000, /* AATR = 0x6F05 */
            0x2C878000, 0x20080000, /* TDR  = 0x80002008 */
        },
        {   /* catch all accesses to zero area */
            0x2C8D0000, 0x00000000, /* ABLR = 0x00000000 */
            0x2C8C0FFF, 0xFFFF0000, /* ABHR = 0x0FFFFFFF */
            0x2C860000, 0xEF050000, /* AATR = 0xEF05 */
            0x2C878000, 0x20080000, /* TDR  = 0x80002008 */
        }
        /* Note: CPU space accesses (movec instruction), interrupt acknowledges
           and emulator mode accesses are never caught. */
    };
    static int cur_mode = MEMGUARD_NONE;

    int oldmode = cur_mode;
    const unsigned long *ptr;
    int i;

    if (newmode == MEMGUARD_KEEP)
        newmode = oldmode;

    /* Always set the new mode, we don't know the old settings
       as we cannot read back */
    ptr = modes[newmode];
    for (i = 0; i < 4; i++)
    {
        asm ( "wdebug (%0) \n" : : "a"(ptr));
        ptr += 2;
    }
    cur_mode = newmode;

    return oldmode;
}

#ifdef IRIVER_H100
#define MAX_REFRESH_TIMER     59
#define NORMAL_REFRESH_TIMER  21
#define DEFAULT_REFRESH_TIMER 4
#else
#define MAX_REFRESH_TIMER     29
#define NORMAL_REFRESH_TIMER  10
#define DEFAULT_REFRESH_TIMER 1
#endif

void set_cpu_frequency (long) __attribute__ ((section (".icode")));
void set_cpu_frequency(long frequency)
{
    switch(frequency)
    {
    case CPUFREQ_MAX:
        DCR = (0x8200 | DEFAULT_REFRESH_TIMER);
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, false);
        PLLCR = 0x11856005;
        CSCR0 = 0x00001180; /* Flash: 4 wait states */
        CSCR1 = 0x00000980; /* LCD: 2 wait states */
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        timers_adjust_prescale(CPUFREQ_MAX_MULT, true);
        DCR = (0x8200 | MAX_REFRESH_TIMER);          /* Refresh timer */
        cpu_frequency = CPUFREQ_MAX;
        IDECONFIG1 = 0x106000 | (5 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (1 << 8); /* TA enable + CS2wait */
        break;
        
    case CPUFREQ_NORMAL:
        DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, false);
        PLLCR = 0x1385e005;
        CSCR0 = 0x00000580; /* Flash: 1 wait state */
        CSCR1 = 0x00000180; /* LCD: 0 wait states */
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        timers_adjust_prescale(CPUFREQ_NORMAL_MULT, true);
        DCR = (0x8000 | NORMAL_REFRESH_TIMER);       /* Refresh timer */
        cpu_frequency = CPUFREQ_NORMAL;
        IDECONFIG1 = 0x106000 | (5 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */
        break;
    default:
        DCR = (DCR & ~0x01ff) | DEFAULT_REFRESH_TIMER;
              /* Refresh timer for bypass frequency */
        PLLCR &= ~1;  /* Bypass mode */
        timers_adjust_prescale(CPUFREQ_DEFAULT_MULT, true);
        PLLCR = 0x10800200; /* Power down PLL, but keep CLSEL and CRSEL */
        CSCR0 = 0x00000180; /* Flash: 0 wait states */
        CSCR1 = 0x00000180; /* LCD: 0 wait states */
        DCR = (0x8000 | DEFAULT_REFRESH_TIMER);       /* Refresh timer */
        cpu_frequency = CPUFREQ_DEFAULT;
        IDECONFIG1 = 0x106000 | (1 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */
        break;
    }
}


#elif CONFIG_CPU == SH7034
#include "led.h"
#include "system.h"
#include "rolo.h"

#define default_interrupt(name,number) \
  extern __attribute__((weak,alias("UIE" #number))) void name (void); void UIE##number (void)
#define reserve_interrupt(number) \
  void UIE##number (void)

static const char* const irqname[] = {
    "", "", "", "", "IllInstr", "", "IllSltIn","","",
    "CPUAdrEr", "DMAAdrEr", "NMI", "UserBrk",
    "","","","","","","","","","","","","","","","","","","",
    "Trap32","Trap33","Trap34","Trap35","Trap36","Trap37","Trap38","Trap39",
    "Trap40","Trap41","Trap42","Trap43","Trap44","Trap45","Trap46","Trap47",
    "Trap48","Trap49","Trap50","Trap51","Trap52","Trap53","Trap54","Trap55",
    "Trap56","Trap57","Trap58","Trap59","Trap60","Trap61","Trap62","Trap63",
    "Irq0","Irq1","Irq2","Irq3","Irq4","Irq5","Irq6","Irq7",
    "Dma0","","Dma1","","Dma2","","Dma3","",
    "IMIA0","IMIB0","OVI0","", "IMIA1","IMIB1","OVI1","",
    "IMIA2","IMIB2","OVI2","", "IMIA3","IMIB3","OVI3","",
    "IMIA4","IMIB4","OVI4","",
    "Ser0Err","Ser0Rx","Ser0Tx","Ser0TE",
    "Ser1Err","Ser1Rx","Ser1Tx","Ser1TE",
    "ParityEr","A/D conv","","","Watchdog","DRAMRefr"
};

reserve_interrupt (          0);
reserve_interrupt (          1);
reserve_interrupt (          2);
reserve_interrupt (          3);
default_interrupt (GII,      4);
reserve_interrupt (          5);
default_interrupt (ISI,      6);
reserve_interrupt (          7);
reserve_interrupt (          8);
default_interrupt (CPUAE,    9);
default_interrupt (DMAAE,   10);
default_interrupt (NMI,     11);
default_interrupt (UB,      12);
reserve_interrupt (         13);
reserve_interrupt (         14);
reserve_interrupt (         15);
reserve_interrupt (         16); /* TCB #0 */
reserve_interrupt (         17); /* TCB #1 */
reserve_interrupt (         18); /* TCB #2 */
reserve_interrupt (         19); /* TCB #3 */
reserve_interrupt (         20); /* TCB #4 */
reserve_interrupt (         21); /* TCB #5 */
reserve_interrupt (         22); /* TCB #6 */
reserve_interrupt (         23); /* TCB #7 */
reserve_interrupt (         24); /* TCB #8 */
reserve_interrupt (         25); /* TCB #9 */
reserve_interrupt (         26); /* TCB #10 */
reserve_interrupt (         27); /* TCB #11 */
reserve_interrupt (         28); /* TCB #12 */
reserve_interrupt (         29); /* TCB #13 */
reserve_interrupt (         30); /* TCB #14 */
reserve_interrupt (         31); /* TCB #15 */
default_interrupt (TRAPA32, 32);
default_interrupt (TRAPA33, 33);
default_interrupt (TRAPA34, 34);
default_interrupt (TRAPA35, 35);
default_interrupt (TRAPA36, 36);
default_interrupt (TRAPA37, 37);
default_interrupt (TRAPA38, 38);
default_interrupt (TRAPA39, 39);
default_interrupt (TRAPA40, 40);
default_interrupt (TRAPA41, 41);
default_interrupt (TRAPA42, 42);
default_interrupt (TRAPA43, 43);
default_interrupt (TRAPA44, 44);
default_interrupt (TRAPA45, 45);
default_interrupt (TRAPA46, 46);
default_interrupt (TRAPA47, 47);
default_interrupt (TRAPA48, 48);
default_interrupt (TRAPA49, 49);
default_interrupt (TRAPA50, 50);
default_interrupt (TRAPA51, 51);
default_interrupt (TRAPA52, 52);
default_interrupt (TRAPA53, 53);
default_interrupt (TRAPA54, 54);
default_interrupt (TRAPA55, 55);
default_interrupt (TRAPA56, 56);
default_interrupt (TRAPA57, 57);
default_interrupt (TRAPA58, 58);
default_interrupt (TRAPA59, 59);
default_interrupt (TRAPA60, 60);
default_interrupt (TRAPA61, 61);
default_interrupt (TRAPA62, 62);
default_interrupt (TRAPA63, 63);
default_interrupt (IRQ0,    64);
default_interrupt (IRQ1,    65);
default_interrupt (IRQ2,    66);
default_interrupt (IRQ3,    67);
default_interrupt (IRQ4,    68);
default_interrupt (IRQ5,    69);
default_interrupt (IRQ6,    70);
default_interrupt (IRQ7,    71);
default_interrupt (DEI0,    72);
reserve_interrupt (         73);
default_interrupt (DEI1,    74);
reserve_interrupt (         75);
default_interrupt (DEI2,    76);
reserve_interrupt (         77);
default_interrupt (DEI3,    78);
reserve_interrupt (         79);
default_interrupt (IMIA0,   80);
default_interrupt (IMIB0,   81);
default_interrupt (OVI0,    82);
reserve_interrupt (         83);
default_interrupt (IMIA1,   84);
default_interrupt (IMIB1,   85);
default_interrupt (OVI1,    86);
reserve_interrupt (         87);
default_interrupt (IMIA2,   88);
default_interrupt (IMIB2,   89);
default_interrupt (OVI2,    90);
reserve_interrupt (         91);
default_interrupt (IMIA3,   92);
default_interrupt (IMIB3,   93);
default_interrupt (OVI3,    94);
reserve_interrupt (         95);
default_interrupt (IMIA4,   96);
default_interrupt (IMIB4,   97);
default_interrupt (OVI4,    98);
reserve_interrupt (         99);
default_interrupt (REI0,   100);
default_interrupt (RXI0,   101);
default_interrupt (TXI0,   102);
default_interrupt (TEI0,   103);
default_interrupt (REI1,   104);
default_interrupt (RXI1,   105);
default_interrupt (TXI1,   106);
default_interrupt (TEI1,   107);
reserve_interrupt (        108);
default_interrupt (ADITI,  109);

/* reset vectors are handled in crt0.S */
void (*vbr[]) (void) __attribute__ ((section (".vectors"))) = 
{
    GII,       /*** 4 General Illegal Instruction ***/
    UIE5,      /*** 5 Reserved ***/
    ISI,       /*** 6 Illegal Slot Instruction ***/
    UIE7,      /*** 7-8 Reserved ***/
    UIE8,
    CPUAE,     /*** 9 CPU Address Error ***/
    DMAAE,     /*** 10 DMA Address Error ***/
    NMI,       /*** 11 NMI ***/
    UB,        /*** 12 User Break ***/

    /*** 13-31 Reserved ***/
    UIE13,UIE14,UIE15,UIE16,UIE17,UIE18,UIE19,UIE20,  
    UIE21,UIE22,UIE23,UIE24,UIE25,UIE26,UIE27,UIE28,
    UIE29,UIE30,UIE31,

    /*** 32-63 TRAPA #20...#3F ***/
    TRAPA32,TRAPA33,TRAPA34,TRAPA35,TRAPA36,TRAPA37,TRAPA38,TRAPA39,
    TRAPA40,TRAPA41,TRAPA42,TRAPA43,TRAPA44,TRAPA45,TRAPA46,TRAPA47,
    TRAPA48,TRAPA49,TRAPA50,TRAPA51,TRAPA52,TRAPA53,TRAPA54,TRAPA55,
    TRAPA56,TRAPA57,TRAPA58,TRAPA59,TRAPA60,TRAPA61,TRAPA62,TRAPA63,

    /*** 64-71 IRQ0-7 ***/ 
    IRQ0,IRQ1,IRQ2,IRQ3,IRQ4,IRQ5,IRQ6,IRQ7,

    DEI0,      /*** 72 DMAC0 ***/
    UIE73,     /*** 73 Reserved ***/
    DEI1,      /*** 74 DMAC1 ***/
    UIE75,     /*** 75 Reserved ***/
    DEI2,      /*** 76 DMAC2 ***/
    UIE77,     /*** 77 Reserved ***/
    DEI3,      /*** 78 DMAC3 ***/
    UIE79,     /*** 79 Reserved ***/
    IMIA0,     /*** 80-82 ITU0 ***/
    IMIB0,
    OVI0,
    UIE83,     /*** 83 Reserved ***/
    IMIA1,     /*** 84-86 ITU1 ***/
    IMIB1,
    OVI1,
    UIE87,     /*** 87 Reserved ***/
    IMIA2,     /*** 88-90 ITU2 ***/
    IMIB2,
    OVI2,
    UIE91,     /*** 91 Reserved ***/
    IMIA3,     /*** 92-94 ITU3 ***/
    IMIB3,
    OVI3,
    UIE95,     /*** 95 Reserved ***/
    IMIA4,     /*** 96-98 ITU4 ***/
    IMIB4,
    OVI4,
    UIE99,     /*** 99 Reserved ***/

    /*** 100-103 SCI0 ***/ 
    REI0,RXI0,TXI0,TEI0,

    /*** 104-107 SCI1 ***/ 
    REI1,RXI1,TXI1,TEI1,

    UIE108,    /*** 108 Parity Control Unit ***/
    ADITI      /*** 109 AD Converter ***/
};


void UIE (unsigned int pc) /* Unexpected Interrupt or Exception */
{
#if CONFIG_LED == LED_REAL
    bool state = true;
#endif
    unsigned int n;
    char str[32];

    asm volatile ("sts\tpr,%0" : "=r"(n));
    
    /* clear screen */
    lcd_clear_display ();
#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);
#endif
    /* output exception */
    n = (n - (unsigned)UIE0 - 4)>>2; /* get exception or interrupt number */
    snprintf(str,sizeof(str),"I%02x:%s",n,irqname[n]);
    lcd_puts(0,0,str);
    snprintf(str,sizeof(str),"at %08x",pc);
    lcd_puts(0,1,str);

#ifdef HAVE_LCD_BITMAP
    lcd_update ();
#endif

    while (1)
    {
#if CONFIG_LED == LED_REAL
        volatile int i;
        led (state);
        state = !state;
        
        for (i = 0; i < 240000; ++i);
#endif

        /* try to restart firmware if ON is pressed */
#if CONFIG_KEYPAD == PLAYER_PAD
        if (!(PADRL & 0x20))
#elif CONFIG_KEYPAD == RECORDER_PAD
#ifdef HAVE_FMADC
        if (!(PCDR & 0x0008))
#else
        if (!(PBDRH & 0x01))
#endif
#elif CONFIG_KEYPAD == ONDIO_PAD
        if (!(PCDR & 0x0008))
#endif
        {   
            /* enable the watchguard timer, but don't service it */
            RSTCSR_W = 0x5a40; /* Reset enabled, power-on reset */
            TCSR_W = 0xa560; /* Watchdog timer mode, timer enabled, sysclk/2 */
        }
    }
}

asm (
    "_UIE0:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE1:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE2:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE3:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE4:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE5:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE6:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE7:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE8:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE9:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE10:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE11:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE12:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE13:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE14:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE15:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE16:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE17:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE18:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE19:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE20:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE21:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE22:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE23:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE24:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE25:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE26:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE27:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE28:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE29:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE30:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE31:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE32:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE33:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE34:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE35:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE36:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE37:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE38:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE39:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE40:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE41:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE42:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE43:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE44:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE45:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE46:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE47:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE48:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE49:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE50:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE51:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE52:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE53:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE54:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE55:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE56:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE57:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE58:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE59:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE60:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE61:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE62:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE63:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE64:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE65:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE66:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE67:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE68:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE69:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE70:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE71:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE72:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE73:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE74:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE75:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE76:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE77:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE78:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE79:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE80:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE81:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE82:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE83:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE84:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE85:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE86:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE87:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE88:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE89:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE90:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE91:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE92:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE93:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE94:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE95:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE96:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE97:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE98:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE99:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE100:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE101:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE102:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE103:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE104:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE105:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE106:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE107:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE108:\tbsr\t_UIE\n\tmov.l\t@r15+,r4\t\n"
    "_UIE109:\tbsr\t_UIE\n\tmov.l\t@r15+,r4");

void system_init(void)
{
    /* Disable all interrupts */
    IPRA = 0;
    IPRB = 0;
    IPRC = 0;
    IPRD = 0;
    IPRE = 0;

    /* NMI level low, falling edge on all interrupts */
    ICR = 0;

    /* Enable burst and RAS down mode on DRAM */
    DCR |= 0x5000;

    /* Activate Warp mode (simultaneous internal and external mem access) */
    BCR |= 0x2000;

    /* Bus state controller initializations. These are only necessary when
       running from flash. */
    WCR1 = 0x40FD; /* Long wait states for CS6 (ATA), short for the rest. */
    WCR3 = 0x8000; /* WAIT is pulled up, 1 state inserted for CS6 */
}

void system_reboot (void)
{
    set_irq_level(HIGHEST_IRQ_LEVEL);

    asm volatile ("ldc\t%0,vbr" : : "r"(0));

    PACR2 |= 0x4000; /* for coldstart detection */
    IPRA = 0;
    IPRB = 0;
    IPRC = 0;
    IPRD = 0;
    IPRE = 0;
    ICR = 0;

    asm volatile ("jmp @%0; mov.l @%1,r15" : :
                  "r"(*(int*)0),"r"(4));
}

/* Utilise the user break controller to catch invalid memory accesses. */
int system_memory_guard(int newmode)
{
    static const struct {
        unsigned long  addr;
        unsigned long  mask;
        unsigned short bbr;
    } modes[MAXMEMGUARD] = {
        /* catch nothing */
        { 0x00000000, 0x00000000, 0x0000 },
        /* catch writes to area 02 (flash ROM) */
        { 0x02000000, 0x00FFFFFF, 0x00F8 },
        /* catch all accesses to areas 00 (internal ROM) and 01 (free) */
        { 0x00000000, 0x01FFFFFF, 0x00FC }
    };

    int oldmode = MEMGUARD_NONE;
    int i;
    
    /* figure out the old mode from what is in the UBC regs. If the register
       values don't match any mode, assume MEMGUARD_NONE */
    for (i = MEMGUARD_NONE; i < MAXMEMGUARD; i++)
    {
        if (BAR == modes[i].addr && BAMR == modes[i].mask &&
            BBR == modes[i].bbr)
        {
            oldmode = i;
            break;
        }
    }
    
    if (newmode == MEMGUARD_KEEP)
        newmode = oldmode;

    BBR = 0;  /* switch off everything first */

    /* always set the UBC according to the mode, in case the old settings
       didn't match any valid mode */
    BAR  = modes[newmode].addr;
    BAMR = modes[newmode].mask;
    BBR  = modes[newmode].bbr;
    
    return oldmode;
}
#elif defined(CPU_ARM)

static const char* const uiename[] = {
    "Undefined instruction", "Prefetch abort", "Data abort"
};

/* Unexpected Interrupt or Exception handler. Currently only deals with
   exceptions, but will deal with interrupts later.
 */
void UIE(unsigned int pc, unsigned int num)
{
    char str[32];
    
    lcd_clear_display();
#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_puts(0, 0, uiename[num]);
    snprintf(str, sizeof(str), "at %08x", pc);
    lcd_puts(0, 1, str);
    lcd_update();
    
    while (1)
    {
        /* TODO: perhaps add button handling in here when we get a polling
           driver some day.
         */
    }
}

#if CONFIG_CPU==PP5020

unsigned int ipod_hw_rev;

#ifndef BOOTLOADER
extern void TIMER1(void);
extern void ipod_4g_button_int(void);

void irq(void)
{
    if (CPU_INT_STAT & TIMER1_MASK) 
        TIMER1();
    else if (CPU_HI_INT_STAT & I2C_MASK)
        ipod_4g_button_int();
}
#endif

/* TODO: The following two function have been lifted straight from IPL, and
   hence have a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations and/or 
   some other CPU frequency scaling. */

#ifndef BOOTLOADER
static void ipod_init_cache(void)
{
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    unsigned i;

    /* cache init mode? */
    outl(0x4, 0x6000C000);

    /* PP5002 has 8KB cache */
    for (i = 0xf0004000; i < 0xf0006000; i += 16) {
        outl(0x0, i);
    }

    outl(0x0, 0xf000f040);
    outl(0x3fc0, 0xf000f044);

    /* enable cache */
    outl(0x1, 0x6000C000);

    for (i = 0x10000000; i < 0x10002000; i += 16)
        inb(i);
}
    
static void ipod_set_cpu_speed(void)
{
    outl(inl(0x70000020) | (1<<30), 0x70000020);

    /* Set run state to 24MHz */
    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000020, 0x60006020);

    /* 75 MHz (24/8)*25 */
    outl(0xaa021908, 0x60006034);
    udelay(2000);

    outl((inl(0x60006020) & 0x0fffff0f) | 0x20000070, 0x60006020);
}
#endif

void system_init(void)
{
#ifndef BOOTLOADER
    /* The hw revision is written to the last 4 bytes of SDRAM by the
       bootloader - we save it before Rockbox overwrites it. */
    ipod_hw_rev = (*((volatile unsigned long*)(0x01fffffc)));

    /* disable all irqs */
    outl(-1, 0x60001138);
    outl(-1, 0x60001128);
    outl(-1, 0x6000111c);

    outl(-1, 0x60001038);
    outl(-1, 0x60001028);
    outl(-1, 0x6000101c);
    ipod_set_cpu_speed();
    ipod_init_cache();
#endif
}

void system_reboot(void)
{
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}
#elif CONFIG_CPU==PP5002
unsigned int ipod_hw_rev;
#ifndef BOOTLOADER
extern void TIMER1(void);

void irq(void)
{
    if (CPU_INT_STAT & TIMER1_MASK) 
        TIMER1();
}

#endif

/* TODO: The following two function have been lifted straight from IPL, and
   hence have a lot of numeric addresses used straight. I'd like to use
   #defines for these, but don't know what most of them are for or even what
   they should be named. Because of this I also have no way of knowing how
   to extend the funtions to do alternate cache configurations and/or 
   some other CPU frequency scaling. */

#ifndef BOOTLOADER
static void ipod_init_cache(void)
{
    int i =0;
/* Initialising the cache in the iPod bootloader prevents Rockbox from starting */
    outl(inl(0xcf004050) & ~0x700, 0xcf004050);
    outl(0x4000, 0xcf004020);

    outl(0x2, 0xcf004024);

    /* PP5002 has 8KB cache */
    for (i = 0xf0004000; i < (int)(0xf0006000); i += 16) {
        outl(0x0, i);
    }

    outl(0x0, 0xf000f020);
    outl(0x3fc0, 0xf000f024);

    outl(0x3, 0xcf004024);
}
    
static void ipod_set_cpu_speed(void)
{
    outl(0x02, 0xcf005008);
    outl(0x55, 0xcf00500c);
    outl(0x6000, 0xcf005010);
#if 1
    // 75  MHz (24/24 * 75) (default)
    outl(24, 0xcf005018);
    outl(75, 0xcf00501c);
#endif

#if 0
    // 66 MHz (24/3 * 8)
    outl(3, 0xcf005018);
    outl(8, 0xcf00501c);
#endif

    outl(0xe000, 0xcf005010);

    udelay(2000);

    outl(0xa8, 0xcf00500c);
}
#endif

void system_init(void)
{
#ifndef BOOTLOADER
    ipod_hw_rev = (*((volatile unsigned long*)(0x01fffffc)));
    outl(-1, 0xcf00101c);
    outl(-1, 0xcf001028);
    outl(-1, 0xcf001038);
    ipod_set_cpu_speed();
    ipod_init_cache();
#endif
}

void system_reboot(void)
{
    outl(inl(0xcf005030) | 0x4, 0xcf005030);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#elif CONFIG_CPU==PNX0101

interrupt_handler_t interrupt_vector[0x1d]  __attribute__ ((section(".idata")));

#define IRQ_REG(reg) (*(volatile unsigned long *)(0x80300000 + (reg)))

static inline unsigned long irq_read(int reg)
{
    unsigned long v, v2;
    do
    {
        v = IRQ_REG(reg);
        v2 = IRQ_REG(reg);
    } while (v != v2);
    return v;
}

#define IRQ_WRITE_WAIT(reg, val, cond) \
    do { unsigned long v, v2; \
        do { \
            IRQ_REG(reg) = (val); \
            v = IRQ_REG(reg); \
            v2 = IRQ_REG(reg); \
        } while ((v != v2) || !(cond)); \
    } while (0);

static void undefined_int(void)
{
}

void irq(void)
{
    int n = irq_read(0x100) >> 3;
    (*(interrupt_vector[n]))();
}

void fiq(void)
{
}

void irq_enable_int(int n)
{
    IRQ_WRITE_WAIT(0x404 + n * 4, 0x4010000, v & 0x10000);
}

void irq_set_int_handler(int n, interrupt_handler_t handler)
{
    interrupt_vector[n + 1] = handler;
}

void system_init(void)
{
    int i;

    /* turn off watchdog */
    (*(volatile unsigned long *)0x80002804) = 0;

    /*
    IRQ_WRITE_WAIT(0x100, 0, v == 0);
    IRQ_WRITE_WAIT(0x104, 0, v == 0);
    IRQ_WRITE_WAIT(0, 0, v == 0);
    IRQ_WRITE_WAIT(4, 0, v == 0);
    */
    
    for (i = 0; i < 0x1c; i++)
    {
        IRQ_WRITE_WAIT(0x404 + i * 4, 0x1e000001, (v & 0x3010f) == 1);
        IRQ_WRITE_WAIT(0x404 + i * 4, 0x4000000, (v & 0x10000) == 0);
        IRQ_WRITE_WAIT(0x404 + i * 4, 0x10000001, (v & 0xf) == 1);
        interrupt_vector[i + 1] = undefined_int;
    }
    interrupt_vector[0] = undefined_int;
}


void system_reboot(void)
{
    (*(volatile unsigned long *)0x80002804) = 1;
    while (1);
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#endif /* CPU_ARM */
#endif /* CONFIG_CPU */

