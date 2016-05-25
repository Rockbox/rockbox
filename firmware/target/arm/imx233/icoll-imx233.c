/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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

#include "icoll-imx233.h"
#include "rtc-imx233.h"
#include "kernel-imx233.h"
#include "string.h"
#include "timrot-imx233.h"

#include "regs/icoll.h"
#include "regs/digctl.h"

/* helpers */
#if IMX233_SUBTARGET >= 3600 && IMX233_SUBTARGET < 3780
#define BP_ICOLL_PRIORITYn_ENABLEx(x)   (2 + 8 * (x))
#define BM_ICOLL_PRIORITYn_ENABLEx(x)   (1 << (2 + 8 * (x)))
#define BP_ICOLL_PRIORITYn_PRIORITYx(x) (0 + 8 * (x))
#define BM_ICOLL_PRIORITYn_PRIORITYx(x) (3 << (0 + 8 * (x)))
#define BF_ICOLL_PRIORITYn_PRIORITYx(x, v)  (((v) << BP_ICOLL_PRIORITYn_PRIORITYx(x)) & BM_ICOLL_PRIORITYn_PRIORITYx(x))
#define BFM_ICOLL_PRIORITYn_PRIORITYx(x, v) BM_ICOLL_PRIORITYn_PRIORITYx(x)
#define BP_ICOLL_PRIORITYn_SOFTIRQx(x)  (3 + 8 * (x))
#define BM_ICOLL_PRIORITYn_SOFTIRQx(x)  (1 << (3 + 8 * (x)))
#endif

#define default_interrupt(name) \
    extern __attribute__((weak, alias("UIRQ"))) void name(void)

static void UIRQ (void) __attribute__((interrupt ("IRQ")));
void irq_handler(void) __attribute__((naked));
void fiq_handler(void) __attribute__((interrupt("FIQ")));

default_interrupt(INT_USB_CTRL);
default_interrupt(INT_TIMER0);
default_interrupt(INT_TIMER1);
default_interrupt(INT_TIMER2);
default_interrupt(INT_TIMER3);
default_interrupt(INT_SSP1_DMA);
default_interrupt(INT_SSP1_ERROR);
default_interrupt(INT_I2C_DMA);
default_interrupt(INT_I2C_ERROR);
default_interrupt(INT_GPIO0);
default_interrupt(INT_GPIO1);
default_interrupt(INT_GPIO2);
default_interrupt(INT_VDD5V);
default_interrupt(INT_LRADC_CH0);
default_interrupt(INT_LRADC_CH1);
default_interrupt(INT_LRADC_CH2);
default_interrupt(INT_LRADC_CH3);
default_interrupt(INT_LRADC_CH4);
default_interrupt(INT_LRADC_CH5);
default_interrupt(INT_LRADC_CH6);
default_interrupt(INT_LRADC_CH7);
default_interrupt(INT_DAC_DMA);
default_interrupt(INT_DAC_ERROR);
default_interrupt(INT_ADC_DMA);
default_interrupt(INT_ADC_ERROR);
default_interrupt(INT_TOUCH_DETECT);
default_interrupt(INT_RTC_1MSEC);
/* STMP3700+ specific */
#if IMX233_SUBTARGET >= 3700
default_interrupt(INT_SSP2_DMA);
default_interrupt(INT_SSP2_ERROR);
default_interrupt(INT_DCP);
default_interrupt(INT_LCDIF_DMA);
default_interrupt(INT_LCDIF_ERROR);
#endif
/* STMP3780+ specific */
#if IMX233_SUBTARGET >= 3780
#endif
default_interrupt(INT_SOFTWARE0);
default_interrupt(INT_SOFTWARE1);
default_interrupt(INT_SOFTWARE2);
default_interrupt(INT_SOFTWARE3);

typedef void (*isr_t)(void);

static isr_t isr_table[INT_SRC_COUNT] =
{
    [INT_SRC_USB_CTRL] = INT_USB_CTRL,
    [INT_SRC_TIMER(0)] = INT_TIMER0,
    [INT_SRC_TIMER(1)] = INT_TIMER1,
    [INT_SRC_TIMER(2)] = INT_TIMER2,
    [INT_SRC_TIMER(3)] = INT_TIMER3,
    [INT_SRC_SSP1_DMA] = INT_SSP1_DMA,
    [INT_SRC_SSP1_ERROR] = INT_SSP1_ERROR,
    [INT_SRC_I2C_DMA] = INT_I2C_DMA,
    [INT_SRC_I2C_ERROR] = INT_I2C_ERROR,
    [INT_SRC_GPIO0] = INT_GPIO0,
    [INT_SRC_GPIO1] = INT_GPIO1,
    [INT_SRC_GPIO2] = INT_GPIO2,
    [INT_SRC_VDD5V] = INT_VDD5V,
    [INT_SRC_LRADC_CHx(0)] = INT_LRADC_CH0,
    [INT_SRC_LRADC_CHx(1)] = INT_LRADC_CH1,
    [INT_SRC_LRADC_CHx(2)] = INT_LRADC_CH2,
    [INT_SRC_LRADC_CHx(3)] = INT_LRADC_CH3,
    [INT_SRC_LRADC_CHx(4)] = INT_LRADC_CH4,
    [INT_SRC_LRADC_CHx(5)] = INT_LRADC_CH5,
    [INT_SRC_LRADC_CHx(6)] = INT_LRADC_CH6,
    [INT_SRC_LRADC_CHx(7)] = INT_LRADC_CH7,
    [INT_SRC_DAC_DMA] = INT_DAC_DMA,
    [INT_SRC_DAC_ERROR] = INT_DAC_ERROR,
    [INT_SRC_ADC_DMA] = INT_ADC_DMA,
    [INT_SRC_ADC_ERROR] = INT_ADC_ERROR,
    [INT_SRC_TOUCH_DETECT] = INT_TOUCH_DETECT,
    [INT_SRC_RTC_1MSEC] = INT_RTC_1MSEC,
#if IMX233_SUBTARGET >= 3700
    [INT_SRC_SSP2_DMA] = INT_SSP2_DMA,
    [INT_SRC_SSP2_ERROR] = INT_SSP2_ERROR,
    [INT_SRC_DCP] = INT_DCP,
    [INT_SRC_LCDIF_DMA] = INT_LCDIF_DMA,
    [INT_SRC_LCDIF_ERROR] = INT_LCDIF_ERROR,
#endif
#if IMX233_SUBTARGET >= 3780
#endif
    [INT_SRC_SOFTWARE(0)] = INT_SOFTWARE0,
    [INT_SRC_SOFTWARE(1)] = INT_SOFTWARE1,
    [INT_SRC_SOFTWARE(2)] = INT_SOFTWARE2,
    [INT_SRC_SOFTWARE(3)] = INT_SOFTWARE3,
};

#define IRQ_STORM_DELAY         100 /* ms */
#define IRQ_STORM_THRESHOLD     100000 /* allows irq / delay */

static uint32_t irq_count_old[INT_SRC_COUNT];
static uint32_t irq_count[INT_SRC_COUNT];
static uint32_t irq_max_time_old[INT_SRC_COUNT];
static uint32_t irq_max_time[INT_SRC_COUNT];
static uint32_t irq_tot_time_old[INT_SRC_COUNT];
static uint32_t irq_tot_time[INT_SRC_COUNT];

unsigned imx233_icoll_get_priority(int src)
{
#if IMX233_SUBTARGET < 3780
    return BF_RD(ICOLL_PRIORITYn(src / 4), PRIORITYx(src % 4));
#else
    return BF_RD(ICOLL_INTERRUPTn(src), PRIORITY);
#endif
}

struct imx233_icoll_irq_info_t imx233_icoll_get_irq_info(int src)
{
    struct imx233_icoll_irq_info_t info;
#if IMX233_SUBTARGET < 3780
    info.enabled = BF_RD(ICOLL_PRIORITYn(src / 4), ENABLEx(src % 4));
#else
    info.enabled = BF_RD(ICOLL_INTERRUPTn(src), ENABLE);
#endif
    info.priority = imx233_icoll_get_priority(src);
    info.freq = irq_count_old[src];
    info.max_time = irq_max_time_old[src];
    info.total_time = irq_tot_time_old[src];
    return info;
}

static void do_irq_stat(void)
{
    imx233_keep_alive();
    static unsigned counter = 0;
    if(counter++ >= HZ)
    {
        counter = 0;
        memcpy(irq_count_old, irq_count, sizeof(irq_count));
        memset(irq_count, 0, sizeof(irq_count));
        memcpy(irq_max_time_old, irq_max_time, sizeof(irq_max_time));
        memset(irq_max_time, 0, sizeof(irq_max_time));
        memcpy(irq_tot_time_old, irq_tot_time, sizeof(irq_tot_time));
        memset(irq_tot_time, 0, sizeof(irq_tot_time));
    }
}

static void UIRQ(void)
{
    panicf("Unhandled IRQ %02X",
        (unsigned int)(HW_ICOLL_VECTOR - (uint32_t)isr_table) / 4);
}

/* return the priority level */
void _irq_handler(void)
{
    /* read vector and notify as side effect */
    uint32_t vec = HW_ICOLL_VECTOR;
    int irq_nr = (vec - HW_ICOLL_VBASE) / 4;
    /* check for IRQ storm */
    if(irq_count[irq_nr]++ > IRQ_STORM_THRESHOLD)
        panicf("IRQ %d: storm detected", irq_nr);
    /* do some regular stat */
    if(irq_nr == INT_SRC_TIMER(TIMER_TICK))
        do_irq_stat();
    /* enable interrupts again */
    //enable_irq();
    uint32_t time = HW_DIGCTL_MICROSECONDS;
    /* process interrupt */
    (*(isr_t *)vec)();
    time = HW_DIGCTL_MICROSECONDS - time;
    irq_max_time[irq_nr] = MAX(irq_max_time[irq_nr], time);
    irq_tot_time[irq_nr] += time;
    /* acknowledge completion of IRQ */
    HW_ICOLL_LEVELACK = 1 << imx233_icoll_get_priority(irq_nr);
}

void irq_handler(void)
{
    /* save stuff */
    asm volatile(
        /* This part is in IRQ mode (with IRQ stack) */
        "sub    lr, lr, #4               \n" /* Create return address */
        "stmfd  sp!, { r0-r5, r12, lr }  \n" /* Save what gets clobbered */
        "ldr    r1, =0x8001c290          \n" /* Save HW_DIGCTL_SCRATCH0 */
        "ldr    r0, [r1]                 \n" /* and store instruction pointer */
        "str    lr, [r1]                 \n" /* in it (for debug) */
        "mrs    r2, spsr                 \n" /* Save SPSR_irq */
        "stmfd  sp!, { r0, r2 }          \n" /* Push it on the stack */
        "msr    cpsr_c, #0x93            \n" /* Switch to SVC mode, IRQ disabled */
        /* This part is in SVC mode (with SVC stack) */
        "msr    spsr_cxsf, r2            \n" /* Copy SPSR_irq to SPSR_svc (for __get_sp) */
        "mov    r4, lr                   \n" /* Save lr_SVC */
        "and    r5, sp, #4               \n" /* Align SVC stack */
        "sub    sp, sp, r5               \n" /* on 8-byte boundary */
        "blx    _irq_handler             \n" /* Process IRQ */
        "add    sp, sp, r5               \n" /* Undo alignement */
        "mov    lr, r4                   \n" /* Restore lr_SVC */
        "msr    cpsr_c, #0x92            \n" /* Mask IRQ, return to IRQ mode */
        /* This part is in IRQ mode (with IRQ stack) */
        "ldmfd  sp!, { r0, lr }          \n" /* Reload saved value */
        "ldr    r1, =0x8001c290          \n" /* Restore HW_DIGCTL_SCRATCH0 */
        "str    r0, [r1]                 \n" /* using saved value */
        "msr    spsr_cxsf, lr            \n" /* Restore SPSR_irq */
        "ldmfd  sp!, { r0-r5, r12, pc }^ \n" /* Restore regs, and RFE */);
}

void fiq_handler(void)
{
}

void imx233_icoll_force_irq(unsigned src, bool enable)
{
#if IMX233_SUBTARGET < 3780
    if(enable)
        BF_SET(ICOLL_PRIORITYn(src / 4), SOFTIRQx(src % 4));
    else
        BF_CLR(ICOLL_PRIORITYn(src / 4), SOFTIRQx(src % 4));
#else
    if(enable)
        BF_SET(ICOLL_INTERRUPTn(src), SOFTIRQ);
    else
        BF_CLR(ICOLL_INTERRUPTn(src), SOFTIRQ);
#endif
}

void imx233_icoll_enable_interrupt(int src, bool enable)
{
#if IMX233_SUBTARGET < 3780
    if(enable)
        BF_SET(ICOLL_PRIORITYn(src / 4), ENABLEx(src % 4));
    else
        BF_CLR(ICOLL_PRIORITYn(src / 4), ENABLEx(src % 4));
#else
    if(enable)
        BF_SET(ICOLL_INTERRUPTn(src), ENABLE);
    else
        BF_CLR(ICOLL_INTERRUPTn(src), ENABLE);
#endif
}

void imx233_icoll_set_priority(int src, unsigned prio)
{
#if IMX233_SUBTARGET < 3780
    BF_WR(ICOLL_PRIORITYn(src / 4), PRIORITYx(src % 4, prio));
#else
    BF_WR(ICOLL_INTERRUPTn(src), PRIORITY(prio));
#endif
}

void imx233_icoll_init(void)
{
    imx233_reset_block(&HW_ICOLL_CTRL);
    /* enable read side-effect mode for nested interrupts */
    BF_SET(ICOLL_CTRL, ARM_RSE_MODE);
    /* disable all interrupts */
    /* priority = 0, disable, disable fiq */
#if IMX233_SUBTARGET >= 3780
    for(int i = 0; i < INT_SRC_COUNT; i++)
        HW_ICOLL_INTERRUPTn(i) = 0;
#else
    for(int i = 0; i < INT_SRC_COUNT / 4; i++)
        HW_ICOLL_PRIORITYn(i) = 0;
#endif
    /* setup vbase as isr_table */
    HW_ICOLL_VBASE = (uint32_t)&isr_table;
    /* enable final irq bit */
    BF_SET(ICOLL_CTRL, IRQ_FINAL_ENABLE);
}
