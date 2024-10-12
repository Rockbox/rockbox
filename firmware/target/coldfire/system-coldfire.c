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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include "config.h"
#include "gcc_extensions.h"
#include "adc.h"
#include "system.h"
#include "lcd.h"
#include "font.h"

#if __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-attributes"
#endif

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIE"))) void name (void)

static const char* const irqname[] = {
    "", "", "AccessErr","AddrErr","IllInstr", "DivX0", "","",
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
    "GPI0","GPI1","GPI2","GPI3","GPI4","GPI5","GPI6","GPI7",
    "","","","","","","","SOFTINT0",
    "SOFTINT1","SOFTINT2","SOFTINT3","",
    "","CDROMCRCERR","CDROMNOSYNC","CDROMILSYNC",
    "CDROMNEWBLK","","","","","","IIC2","ADC",
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

default_interrupt (IIC2); /* I2C 2 */
default_interrupt (ADC);  /* A/D converter */

#if defined(IAUDIO_X5) || defined(IAUDIO_M5)
#define EXCP_BUTTON_GPIO_READ   GPIO_READ
#define EXCP_BUTTON_MASK        0x0c000000
#define EXCP_BUTTON_VALUE       0x08000000 /* On button and !hold */
#define EXCP_PLLCR              0x10400000
#elif defined(IAUDIO_M3)
#define EXCP_BUTTON_GPIO_READ   GPIO1_READ
#define EXCP_BUTTON_MASK        0x00000202
#define EXCP_BUTTON_VALUE       0x00000200 /* On button and !hold */
#define EXCP_PLLCR              0x10800000
#elif defined(MPIO_HD200)
#define EXCP_BUTTON_GPIO_READ   GPIO1_READ
#define EXCP_BUTTON_MASK        0x01000010
#define EXCP_BUTTON_VALUE       0x01000000 /* Play button and !hold */
#define EXCP_PLLCR              0x10800000
#elif defined(MPIO_HD300)
#define EXCP_BUTTON_GPIO_READ GPIO1_READ
#define EXCP_BUTTON_MASK        0x01080000
#define EXCP_BUTTON_VALUE       0x01080000 /* Play button and !hold */
#define EXCP_PLLCR              0x10800000
#else
#define EXCP_BUTTON_GPIO_READ   GPIO1_READ
#define EXCP_BUTTON_MASK        0x00000022
#define EXCP_BUTTON_VALUE       0x00000000
#define EXCP_PLLCR              0x10800000
#endif

static void system_display_exception_info(unsigned long format,
    unsigned long pc) NORETURN_ATTR USED_ATTR;
static void system_display_exception_info(unsigned long format,
                                          unsigned long pc)
{
    int vector = (format >> 18) & 0xff;

    /* clear screen */
#if LCD_DEPTH > 1
    lcd_set_backdrop(NULL);
    lcd_set_drawmode(DRMODE_SOLID);
    lcd_set_foreground(LCD_BLACK);
    lcd_set_background(LCD_WHITE);
#endif
    lcd_setfont(FONT_SYSFIXED);
    lcd_set_viewport(NULL);
    lcd_clear_display();

    lcd_putsf(0, 0, "I%02x:%s", vector, irqname[vector]);
    lcd_putsf(0, 1, "at %08x", pc);
    lcd_update();

    system_exception_wait();

    /* Start watchdog timer with 512 cycles timeout. Don't service it. */
    SYPCR = 0xc0;
    while (1);

    /* We need a reset method that works in all cases. Calling system_reboot()
       doesn't work when we're called from the debug interrupt, because then
       the CPU is in emulator mode and the only ways leaving it are exexcuting
       an rte instruction or performing a reset. Even disabling the breakpoint
       logic and performing special rte magic doesn't make system_reboot()
       reliable. The system restarts, but boot often fails with ata error -42. */
}

static void UIE(void) NORETURN_ATTR;
static void UIE(void)
{
    asm volatile("subq.l #4,%sp"); /* phony return address - never used */
    asm volatile("jmp    system_display_exception_info");
    while (1); /* loop to silence 'noreturn' function does return */
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
    CDROMNEWBLK,UIE,UIE,UIE,UIE,UIE,IIC2,ADC,

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
       whoever uses them to clear them after use and before giving control
       to "foreign" code (use movclr instruction). */
    asm volatile ("movclr.l %%acc0, %%d0\n\t"
                  "movclr.l %%acc1, %%d0\n\t"
                  "movclr.l %%acc2, %%d0\n\t"
                  "movclr.l %%acc3, %%d0\n\t"
                  : : : "d0");

    /* Set EMAC unit to fractional mode with saturation, since that's
       what'll be the most useful for most things which the main thread
       will do. */
    coldfire_set_macsr(EMAC_FRACTIONAL | EMAC_SATURATE);
    
    coldfire_imr_mod(0x3ffff, 0x3ffff);
    INTPRI1 = 0;
    INTPRI2 = 0;
    INTPRI3 = 0;
    INTPRI4 = 0;
    INTPRI5 = 0;
    INTPRI6 = 0;
    INTPRI7 = 0;
    INTPRI8 = 0;

    /* Set INTBASE and SPURVEC */
    INTBASE = 64;
    SPURVEC = 24;

    MPARK     = 0x81;   /* PARK[1,0]=10 + BCR24BIT                   */
    
#ifndef HAVE_ADJUSTABLE_CPU_FREQ
    cf_set_cpu_frequency(CPUFREQ_DEFAULT);
#endif
}

void system_reboot (void)
{
    adc_close();
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

void system_exception_wait(void)
{
    /* set cpu frequency to 11mhz (to prevent overheating) */
    DCR = (DCR & ~0x01ff) | 1;
    PLLCR = EXCP_PLLCR;
    while ((EXCP_BUTTON_GPIO_READ & EXCP_BUTTON_MASK) != EXCP_BUTTON_VALUE);
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

/* allow setting of audio clock related bits */
void coldfire_set_pllcr_audio_bits(long bits)
{
    PLLCR = (PLLCR & ~0x70400000) | (bits & 0x70400000);
}

/* Safely modify the interrupt mask register as the core interrupt level is
   required to be at least as high as the level interrupt being
   masked/unmasked */
void coldfire_imr_mod(unsigned long bits, unsigned long mask)
{
    unsigned long oldlevel = set_irq_level(DISABLE_INTERRUPTS);
    IMR = (IMR & ~mask) | (bits & mask);
    restore_irq(oldlevel);
}

/* Set DATAINCONTROL without disturbing FIFO reset state */
void coldfire_set_dataincontrol(unsigned long value)
{
    /* Have to be atomic against recording stop initiated by DMA1 */
    int level = set_irq_level(DMA_IRQ_LEVEL);
    DATAINCONTROL = (DATAINCONTROL & (1 << 9)) | value;
    restore_irq(level);
}

void commit_discard_idcache(void)
{
   asm volatile ("move.l #0x01000000,%d0\n"
                 "movec.l %d0,%cacr\n"
                 "move.l #0x80000000,%d0\n"
                 "movec.l %d0,%cacr");
}
