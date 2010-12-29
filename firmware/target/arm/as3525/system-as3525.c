/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Rob Purchase
 * Copyright © 2008 Rafaël Carré
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

#include "config.h"
#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "ascodec-target.h"
#include "adc.h"
#include "dma-target.h"
#include "clock-target.h"
#include "fmradio_i2c.h"
#include "button-target.h"
#include "backlight-target.h"
#include "lcd.h"

#define default_interrupt(name) \
  extern __attribute__((weak,alias("UIRQ"))) void name (void)

static void UIRQ (void) __attribute__((interrupt ("IRQ")));
void irq_handler(void) __attribute__((interrupt ("IRQ")));
void fiq_handler(void) __attribute__((interrupt ("FIQ")));

default_interrupt(INT_WATCHDOG);
default_interrupt(INT_TIMER1);
default_interrupt(INT_TIMER2);
default_interrupt(INT_USB);
default_interrupt(INT_DMAC);
default_interrupt(INT_NAND);
default_interrupt(INT_IDE);
default_interrupt(INT_MCI0);
default_interrupt(INT_MCI1);
default_interrupt(INT_AUDIO);
default_interrupt(INT_SSP);
default_interrupt(INT_I2C_MS);
default_interrupt(INT_I2C_AUDIO);
default_interrupt(INT_I2SIN);
default_interrupt(INT_I2SOUT);
default_interrupt(INT_UART);
default_interrupt(INT_GPIOD);
default_interrupt(RESERVED1); /* Interrupt 17 : unused */
default_interrupt(INT_CGU);
default_interrupt(INT_MEMORY_STICK);
default_interrupt(INT_DBOP);
default_interrupt(RESERVED2); /* Interrupt 21 : unused */
default_interrupt(RESERVED3); /* Interrupt 22 : unused */
default_interrupt(RESERVED4); /* Interrupt 23 : unused */
default_interrupt(RESERVED5); /* Interrupt 24 : unused */
default_interrupt(RESERVED6); /* Interrupt 25 : unused */
default_interrupt(RESERVED7); /* Interrupt 26 : unused */
default_interrupt(RESERVED8); /* Interrupt 27 : unused */
default_interrupt(RESERVED9); /* Interrupt 28 : unused */
/* INT_GPIOA is declared in this file */
void INT_GPIOA(void);
default_interrupt(INT_GPIOB);
default_interrupt(INT_GPIOC);

static const char * const irqname[] =
{
    "INT_WATCHDOG", "INT_TIMER1", "INT_TIMER2", "INT_USB", "INT_DMAC", "INT_NAND",
    "INT_IDE", "INT_MCI0", "INT_MCI1", "INT_AUDIO", "INT_SSP", "INT_I2C_MS",
    "INT_I2C_AUDIO", "INT_I2SIN", "INT_I2SOUT", "INT_UART", "INT_GPIOD", "RESERVED1",
    "INT_CGU", "INT_MEMORY_STICK", "INT_DBOP", "RESERVED2", "RESERVED3", "RESERVED4",
    "RESERVED5", "RESERVED6", "RESERVED7", "RESERVED8", "RESERVED9", "INT_GPIOA",
    "INT_GPIOB", "INT_GPIOC"
};

static void UIRQ(void)
{
    bool masked = false;
    int status = VIC_IRQ_STATUS;
    if(status == 0)
    {
        status = VIC_RAW_INTR; /* masked interrupts */
        masked = true;
    }

    if(status == 0)
        panicf("Unhandled IRQ (source unknown!)");

    unsigned irq_no = 31 - __builtin_clz(status);

    panicf("Unhandled %smasked IRQ %02X: %s (status 0x%8X)",
           masked ? "" : "un", irq_no, irqname[irq_no], status);
}

struct vec_int_src
{
    int source;
    void (*isr) (void);
};

/* Vectored interrupts (16 available) */
struct vec_int_src vec_int_srcs[] =
{
    /* Highest priority at the top of the list */
    { INT_SRC_DMAC, INT_DMAC },
    { INT_SRC_NAND, INT_NAND },
#if (defined HAVE_MULTIDRIVE  && CONFIG_CPU == AS3525)
    { INT_SRC_MCI0, INT_MCI0 },
#endif
    { INT_SRC_USB, INT_USB, },
#ifdef HAVE_RECORDING
    { INT_SRC_I2SIN, INT_I2SIN, },
#endif
    { INT_SRC_TIMER1, INT_TIMER1 },
    { INT_SRC_TIMER2, INT_TIMER2 },
    { INT_SRC_I2C_AUDIO, INT_I2C_AUDIO },
    { INT_SRC_AUDIO, INT_AUDIO },
#if defined(HAVE_HOTSWAP) || \
    (defined(SANSA_FUZEV2) && !INCREASED_SCROLLWHEEL_POLLING)
    { INT_SRC_GPIOA, INT_GPIOA, },
#endif
    /* Lowest priority at the end of the list */
};

static void setup_vic(void)
{
    const unsigned int n = sizeof(vec_int_srcs)/sizeof(vec_int_srcs[0]);
    unsigned int i;

    CGU_PERI |= CGU_VIC_CLOCK_ENABLE; /* enable VIC */
    VIC_INT_EN_CLEAR = 0xffffffff; /* disable all interrupt lines */
    VIC_INT_SELECT = 0; /* only IRQ, no FIQ */

    *VIC_DEF_VECT_ADDR = UIRQ;

    for(i = 0; i < n; i++)
    {
        VIC_VECT_ADDRS[i] = vec_int_srcs[i].isr;
        VIC_VECT_CNTLS[i] = (1<<5) | vec_int_srcs[i].source;
    }
}

void INT_GPIOA(void)
{
#ifdef HAVE_HOTSWAP
    void sd_gpioa_isr(void);
    sd_gpioa_isr();
#endif
#if defined(SANSA_FUZEV2) && !INCREASED_SCROLLWHEEL_POLLING
    void button_gpioa_isr(void);
    button_gpioa_isr();
#endif
}

void irq_handler(void)
{
    (*VIC_VECT_ADDR)(); /* call the isr */
    *VIC_VECT_ADDR = (void*)VIC_VECT_ADDR; /* any write will ack the irq */
}

void fiq_handler(void)
{
}

#if defined(SANSA_C200V2)
int c200v2_variant;

static void check_model_variant(void)
{
    unsigned int i;
    unsigned int saved_dir = GPIOA_DIR;

    /* Make A7 input */
    GPIOA_DIR &= ~(1<<7);
    /* wait a little to allow the pullup/pulldown resistor
     * to charge the input capacitance */
    for (i=0; i<1000; i++) asm volatile ("nop\n");
    /* read the pullup/pulldown value on A7 to determine the variant */
    c200v2_variant = !GPIOA_PIN(7);
    GPIOA_DIR = saved_dir;
}
#elif defined(SANSA_FUZEV2) || defined(SANSA_CLIPPLUS)
int amsv2_variant;

static void check_model_variant(void)
{
    GPIOB_DIR &= ~(1<<5);
    amsv2_variant = !!GPIOB_PIN(5);
}
#else
static inline void check_model_variant(void)
{
}
#endif /* model selection */

void system_init(void)
{
#if CONFIG_CPU == AS3525v2
    CCU_SRC = 0x57D7BF0;
#else
    CCU_SRC = 0x1fffff0
        & ~CCU_SRC_IDE_EN; /* FIXME */
#endif

    unsigned int reset_loops = 640;
    while(reset_loops--)
        CCU_SRL = CCU_SRL_MAGIC_NUMBER;
    CCU_SRC = CCU_SRL = 0;

    CCU_SCON = 1; /* AHB master's priority configuration :
                     TIC (Test Interface Controller) > DMA > USB > IDE > ARM */

    CGU_PROC = 0;           /* fclk 24 MHz */
#if CONFIG_CPU == AS3525v2
    /* pclk is always based on PLLA, since we don't know the current PLLA speed,
     * avoid having pclk too fast and hope it's not too low */
    CGU_PERI |= 0xf << 2;   /* pclk lowest */
#else
    CGU_PERI &= ~0x7f;      /* pclk 24 MHz */
#endif

    /* bits 31:30 should be set to 0 in arm926-ejs */
    asm volatile(
        "mrc p15, 0, r0, c1, c0   \n"      /* control register */
        "bic r0, r0, #3<<30       \n"      /* clears bus bits : sets fastbus */
        "mcr p15, 0, r0, c1, c0   \n"
        : : : "r0" );

    CGU_COUNTA = CGU_LOCK_CNT;
    CGU_PLLA = AS3525_PLLA_SETTING;
    CGU_PLLASUP = 0;                          /* enable PLLA */
    while(!(CGU_INTCTRL & CGU_PLLA_LOCK));    /* wait until PLLA is locked */

#if AS3525_MCLK_SEL == AS3525_CLK_PLLB
    CGU_COUNTB = CGU_LOCK_CNT;
    CGU_PLLB = AS3525_PLLB_SETTING;
    CGU_PLLBSUP = 0;                          /* enable PLLB */
    while(!(CGU_INTCTRL & CGU_PLLB_LOCK));    /* wait until PLLB is locked */
#endif

    /*  Set FCLK frequency */
    CGU_PROC = ((AS3525_FCLK_POSTDIV << 4) |
                (AS3525_FCLK_PREDIV  << 2) |
                 AS3525_FCLK_SEL);

    /*  Set PCLK frequency */
    CGU_PERI = ((CGU_PERI & ~0x7F)  |       /* reset divider & clksel bits */
                 (AS3525_PCLK_DIV0 << 2) |
#if CONFIG_CPU == AS3525
                 (AS3525_PCLK_DIV1 << 6) |
#endif
                  AS3525_PCLK_SEL);

    set_cpu_frequency(CPUFREQ_DEFAULT);

#if 0 /* the GPIO clock is already enabled by the dualboot function */
    CGU_PERI |= CGU_GPIO_CLOCK_ENABLE;
#endif

    /* enable timer interface for TIMER1 & TIMER2 */
    CGU_PERI |= CGU_TIMERIF_CLOCK_ENABLE;

    setup_vic();

    dma_init();

    ascodec_init();

#ifndef BOOTLOADER
    /* setup isr for microsd monitoring and for fuzev2 scrollwheel irq */
#if defined(HAVE_HOTSWAP) || \
    (defined(SANSA_FUZEV2) && !INCREASED_SCROLLWHEEL_POLLING)
    VIC_INT_ENABLE = (INTERRUPT_GPIOA);
    /* pin selection for irq happens in the drivers */
#endif

    /*  Initialize power management settings */
    ascodec_write(AS3514_CVDD_DCDC3, AS314_CP_DCDC3_SETTING);
#if CONFIG_TUNER
    fmradio_i2c_init();
#endif
#endif /* !BOOTLOADER */
    check_model_variant();
}

void system_reboot(void)
{
    _backlight_off();

    disable_irq();

    /* use watchdog to reset */
    CGU_PERI |= (CGU_WDOCNT_CLOCK_ENABLE | CGU_WDOIF_CLOCK_ENABLE);
    WDT_LOAD = 1; /* set counter to 1 */
    WDT_CONTROL = 3; /* enable watchdog counter & reset */
    while(1);
}

void system_exception_wait(void)
{
    /* make sure lcd+backlight are on */
    _backlight_panic_on();
    /* make sure screen content is up to date */
    lcd_update();
    /* wait until button release (if a button is pressed) */
    while(button_read_device());
    /* then wait until next button press */
    while(!button_read_device());
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

/* usecs may be at most 2^32/248 (17 seconds) for 248MHz max cpu freq */
void udelay(unsigned usecs)
{
    unsigned cycles_per_usec;
    unsigned delay;

    if (cpu_frequency == CPUFREQ_MAX) {
        cycles_per_usec = (CPUFREQ_MAX + 999999) / 1000000;
    } else {
        cycles_per_usec = (CPUFREQ_NORMAL + 999999) / 1000000;
    }

    delay = (usecs * cycles_per_usec + 3) / 4;

    asm volatile(
        "1: subs %0, %0, #1  \n"    /* 1 cycle  */
        "   bne  1b          \n"    /* 3 cycles */
        : : "r"(delay)
    );
}

#ifndef BOOTLOADER
#ifdef HAVE_ADJUSTABLE_CPU_FREQ

#if CONFIG_CPU == AS3525
void set_cpu_frequency(long frequency)
{
    if(frequency == CPUFREQ_MAX)
    {
#ifdef HAVE_ADJUSTABLE_CPU_VOLTAGE
        /* Increasing frequency so boost voltage before change */
        ascodec_write(AS3514_CVDD_DCDC3, (AS314_CP_DCDC3_SETTING | CVDD_1_20));

        /* Some players run a bit low so use 1.175 volts instead of 1.20  */
        /* Wait for voltage to be at least 1.175v before making fclk > 200 MHz */
        while(adc_read(ADC_CVDD) < 470); /* 470 * .0025 = 1.175V */
#endif  /*  HAVE_ADJUSTABLE_CPU_VOLTAGE */

        CGU_PROC = ((AS3525_FCLK_POSTDIV << 4) |
                    (AS3525_FCLK_PREDIV  << 2) |
                     AS3525_FCLK_SEL);

        asm volatile(
            "mrc p15, 0, r0, c1, c0  \n"
            "orr r0, r0, #3<<30      \n"   /* asynchronous bus clocking */
            /* synchronous bus clocking had issues on some players */
            "mcr p15, 0, r0, c1, c0  \n"
            : : : "r0" );

        cpu_frequency = CPUFREQ_MAX;
    }
    else
    {
        asm volatile(
            "mrc p15, 0, r0, c1, c0  \n"
            "bic r0, r0, #3<<30      \n"     /* fastbus clocking */
            "mcr p15, 0, r0, c1, c0  \n"
            : : : "r0" );

        /* FCLK is unused so put it to the lowest freq we can */
        CGU_PROC = ((0xf << 4) | (0x3 << 2) | AS3525_CLK_MAIN);

#ifdef HAVE_ADJUSTABLE_CPU_VOLTAGE
        /* Decreasing frequency so reduce voltage after change */
        ascodec_write(AS3514_CVDD_DCDC3, (AS314_CP_DCDC3_SETTING | CVDD_1_10));
#endif  /*  HAVE_ADJUSTABLE_CPU_VOLTAGE */

        cpu_frequency = CPUFREQ_NORMAL;
    }
}
#else   /* as3525v2  */
/* FIXME : disabled for now, seems to cause buggy memory accesses
 * Disabling MMU or putting the function in uncached memory seems to help? */
void set_cpu_frequency(long frequency)
{
    int oldstatus = disable_irq_save();

    /* We only have 2 settings */
    cpu_frequency = (frequency == CPUFREQ_MAX) ? frequency : CPUFREQ_NORMAL;

    if(frequency == CPUFREQ_MAX)
    {
        /* Change PCLK while FCLK is low, so it doesn't go too high */
        CGU_PERI = (CGU_PERI & ~(0xF << 2)) | (AS3525_PCLK_DIV0 << 2);

        CGU_PROC = ((AS3525_FCLK_POSTDIV << 4) |
                    (AS3525_FCLK_PREDIV  << 2) |
                    AS3525_FCLK_SEL);
    }
    else
    {
        CGU_PROC = ((AS3525_FCLK_POSTDIV_UNBOOSTED << 4) |
                    (AS3525_FCLK_PREDIV  << 2) |
                    AS3525_FCLK_SEL);

        /* Change PCLK after FCLK is low, so it doesn't go too high */
        CGU_PERI = (CGU_PERI & ~(0xF << 2)) | (AS3525_PCLK_DIV0_UNBOOSTED << 2);
    }

    restore_irq(oldstatus);
}
#endif

#endif /* HAVE_ADJUSTABLE_CPU_FREQ */
#endif /* !BOOTLOADER */
