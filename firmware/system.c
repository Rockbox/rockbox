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

#ifndef SIMULATOR
long cpu_frequency = CPU_FREQ;
#endif

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
int boost_counter = 0;
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
            set_cpu_frequency(CPUFREQ_NORMAL);
        }

        /* Safety measure */
        if(boost_counter < 0)
            boost_counter = 0;
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



#elif CONFIG_CPU == MCF5249

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIE"))) void name (void);

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

    while (1)
    {
    }
}

/* reset vectors are handled in crt0.S */
void (* const vbr[]) (void) __attribute__ ((section (".vectors"))) = 
{
    UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,UIE,UIE,UIE,UIE,UIE,
    UIE,UIE,UIE,TIMER0,UIE,UIE,UIE,UIE,

    TRAP0,TRAP1,TRAP2,TRAP3,TRAP4,TRAP5,TRAP6,TRAP7,
    TRAP8,TRAP9,TRAP10,TRAP11,TRAP12,TRAP13,TRAP14,TRAP15,
    
    SWT,UIE,TIMER1,I2C,UART1,UART2,DMA0,DMA1,
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

void set_cpu_frequency (long) __attribute__ ((section (".icode")));
void set_cpu_frequency(long frequency)
{
    switch(frequency)
    {
    case CPUFREQ_MAX:
        DCR = (DCR & ~0x000001ff) | 1;   /* Refresh timer for bypass
                                            frequency */
        PLLCR &= ~1;  /* Bypass mode */
        PLLCR = 0x11853005;
        CSCR0 = 0x00000980; /* Flash: 2 wait state */
        CSCR1 = 0x00002580; /* LCD: 9 wait states */
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        DCR = (DCR & ~0x000001ff) | 28;   /* Refresh timer */
        cpu_frequency = CPUFREQ_MAX;
        tick_start(1000/HZ);
        IDECONFIG1 = 0x106000 | (5 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (1 << 8); /* TA enable + CS2wait */
        /* I2C Clock divisor = 1280 => 119.952 MHz / 1280 = 93,7 kHz */
        MFDR = 0x19;  
        MFDR2 = 0x19;
        break;
        
    case CPUFREQ_NORMAL:
        DCR = (DCR & ~0x000001ff) | 1;   /* Refresh timer for bypass
                                            frequency */
        PLLCR &= ~1;  /* Bypass mode */
        PLLCR = 0x10886001;
        CSCR0 = 0x00000180; /* Flash: 0 wait states */
        CSCR1 = 0x00000980; /* LCD: 2 wait states */
        while(!(PLLCR & 0x80000000)) {}; /* Wait until the PLL has locked.
                                            This may take up to 10ms! */
        DCR = (DCR & ~0x000001ff) | 10;   /* Refresh timer */
        cpu_frequency = CPUFREQ_NORMAL;
        tick_start(1000/HZ);
        IDECONFIG1 = 0x106000 | (5 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */
        /* I2C Clock divisor = 480 => 47.9808 MHz / 480 = 99,9 kHz */
        MFDR = 0x13;  
        MFDR2 = 0x13;
        break;
    default:
        DCR = (DCR & ~0x000001ff) | 1;   /* Refresh timer for bypass
                                            frequency */
        PLLCR = 0x00000000;  /* Bypass mode */
        CSCR0 = 0x00000180; /* Flash: 0 wait states */
        CSCR1 = 0x00000180; /* LCD: 0 wait states */
        cpu_frequency = CPU_FREQ;
        tick_start(1000/HZ);
        IDECONFIG1 = 0x106000 | (1 << 10); /* BUFEN2 enable + CS2Pre/CS2Post */
        IDECONFIG2 = 0x40000 | (0 << 8); /* TA enable + CS2wait */
        /* I2C Clock divisor = 480 => 47.9808 MHz / 480 = 99,9 kHz */
        MFDR = 0x13;
        MFDR2 = 0x13;
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
    /*** 4 General Illegal Instruction ***/

    GII,

    /*** 5 Reserved ***/

    UIE5,

    /*** 6 Illegal Slot Instruction ***/

    ISI,

    /*** 7-8 Reserved ***/

    UIE7,UIE8,

    /*** 9 CPU Address Error ***/

    CPUAE,

    /*** 10 DMA Address Error ***/

    DMAAE,

    /*** 11 NMI ***/

    NMI,

    /*** 12 User Break ***/

    UB,

    /*** 13-31 Reserved ***/

    UIE13,UIE14,UIE15,UIE16,UIE17,UIE18,UIE19,UIE20,UIE21,UIE22,UIE23,UIE24,UIE25,UIE26,UIE27,UIE28,UIE29,UIE30,UIE31,
  
    /*** 32-63 TRAPA #20...#3F ***/

    TRAPA32,TRAPA33,TRAPA34,TRAPA35,TRAPA36,TRAPA37,TRAPA38,TRAPA39,TRAPA40,TRAPA41,TRAPA42,TRAPA43,TRAPA44,TRAPA45,TRAPA46,TRAPA47,TRAPA48,TRAPA49,TRAPA50,TRAPA51,TRAPA52,TRAPA53,TRAPA54,TRAPA55,TRAPA56,TRAPA57,TRAPA58,TRAPA59,TRAPA60,TRAPA61,TRAPA62,TRAPA63,
  
    /*** 64-71 IRQ0-7 ***/ 

    IRQ0,IRQ1,IRQ2,IRQ3,IRQ4,IRQ5,IRQ6,IRQ7,
  
    /*** 72 DMAC0 ***/ 
  
    DEI0,
    
    /*** 73 Reserved ***/

    UIE73,

    /*** 74 DMAC1 ***/ 

    DEI1,
    
    /*** 75 Reserved ***/

    UIE75,

    /*** 76 DMAC2 ***/ 

    DEI2,
    
    /*** 77 Reserved ***/

    UIE77,

    /*** 78 DMAC3 ***/ 

    DEI3,
    
    /*** 79 Reserved ***/

    UIE79, 
  
    /*** 80-82 ITU0 ***/
  
    IMIA0,IMIB0,OVI0,
    
    /*** 83 Reserved ***/

    UIE83, 

    /*** 84-86 ITU1 ***/
  
    IMIA1,IMIB1,OVI1,
    
    /*** 87 Reserved ***/

    UIE87, 
  
    /*** 88-90 ITU2 ***/
  
    IMIA2,IMIB2,OVI2,
    
    /*** 91 Reserved ***/

    UIE91, 
  
    /*** 92-94 ITU3 ***/
  
    IMIA3,IMIB3,OVI3,
    
    /*** 95 Reserved ***/

    UIE95, 
  
    /*** 96-98 ITU4 ***/
  
    IMIA4,IMIB4,OVI4,
    
    /*** 99 Reserved ***/

    UIE99,
    
    /*** 100-103 SCI0 ***/ 
  
    REI0,RXI0,TXI0,TEI0,
  
    /*** 104-107 SCI1 ***/ 
  
    REI1,RXI1,TXI1,TEI1,
  
    /*** 108 Parity Control Unit ***/
  
    UIE108,       
  
    /*** 109 AD Converter ***/
  
    ADITI
    
};


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

void UIE (unsigned int pc) /* Unexpected Interrupt or Exception */
{
    bool state = true;
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
        volatile int i;
        led (state);
        state = state?false:true;
        
        for (i = 0; i < 240000; ++i);

        /* try to restart firmware if ON is pressed */
#if CONFIG_KEYPAD == PLAYER_PAD
        if (!(PADR & 0x0020))
            rolo_load("/archos.mod");
#elif CONFIG_KEYPAD == RECORDER_PAD
#ifdef HAVE_FMADC
        if (!(PCDR & 0x0008))
#else
        if (!(PBDR & 0x0100))
#endif
            rolo_load("/ajbrec.ajz");
#endif
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

/* Utilize the user break controller to catch invalid memory accesses. */
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
#endif

#if CONFIG_CPU != SH7034
/* this does nothing on non-SH systems */
int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

void system_reboot (void)
{
#if CONFIG_CPU == MCF5249
    set_cpu_frequency(0);
    
    asm(" move.w #0x2700,%sr");
    /* Reset the cookie for the crt0 crash check */
    asm(" move.l #0,%d0");
    asm(" move.l %d0,0x10017ffc");
    asm(" movec.l %d0,%vbr");
    asm(" move.l 0,%sp");
    asm(" move.l 4,%a0");
    asm(" jmp (%a0)");
#endif
}
#endif
