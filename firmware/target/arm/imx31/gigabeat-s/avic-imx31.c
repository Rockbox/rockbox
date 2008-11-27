/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by James Espinoza
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
#include "system.h"
#include "imx31l.h"
#include "avic-imx31.h"
#include "panic.h"
#include "debug.h"

static const char * avic_int_names[64] =
{
    "RESERVED0", "RESERVED1",     "RESERVED2", "I2C3",
    "I2C2",      "MPEG4_ENCODER", "RTIC",      "FIR",
    "MMC/SDHC2", "MMC/SDHC1",     "I2C1",      "SSI2",
    "SSI1",      "CSPI2",         "CSPI1",     "ATA",
    "MBX",       "CSPI3",         "UART3",     "IIM",
    "SIM1",      "SIM2",          "RNGA",      "EVTMON",
    "KPP",       "RTC",           "PWN",       "EPIT2",
    "EPIT1",     "GPT",           "PWR_FAIL",  "CCM_DVFS",
    "UART2",     "NANDFC",        "SDMA",      "USB_HOST1",
    "USB_HOST2", "USB_OTG",       "RESERVED3", "MSHC1",
    "MSHC2",     "IPU_ERR",       "IPU",       "RESERVED4",
    "RESERVED5", "UART1",         "UART4",     "UART5",
    "ETC_IRQ",   "SCC_SCM",       "SCC_SMN",   "GPIO2",
    "GPIO1",     "CCM_CLK",       "PCMCIA",    "WDOG",
    "GPIO3",     "RESERVED6",     "EXT_PWMG",  "EXT_TEMP",
    "EXT_SENS1", "EXT_SENS2",     "EXT_WDOG",  "EXT_TV"
};

void UIE_VECTOR(void)
{
    int mode;
    int offset;

    asm volatile (
        "mrs    %0, cpsr      \n" /* Mask core IRQ/FIQ */
        "orr    %0, %0, #0xc0 \n"
        "msr    cpsr_c, %0    \n"
        "and    %0, %0, #0x1f \n" /* Get mode bits */
        : "=&r"(mode)
    );

    offset = mode == 0x11 ?
        (int32_t)FIVECSR : ((int32_t)NIVECSR >> 16);

    panicf("Unhandled %s %d: %s",
           mode == 0x11 ? "FIQ" : "IRQ", offset,
           offset >= 0 ? avic_int_names[offset] : "<Unknown>");
}

/* We use the AVIC */
void __attribute__((interrupt("IRQ"))) irq_handler(void)
{
    const int offset = (int32_t)NIVECSR >> 16;

    if (offset == -1)
    {
        /* This is called occasionally for some unknown reason even with the
         * avic enabled but returning normally appears to cause no harm. The
         * KPP and ATA seem to have a part in it (common but multiplexed pins
         * that can interfere). It will be investigated more thoroughly but
         * for now it is simply an occasional irritant. */
        return;
    }

    disable_interrupt(IRQ_FIQ_STATUS);
    panicf("Unhandled IRQ %d in irq_handler: %s", offset,
           offset >= 0 ? avic_int_names[offset] : "<Unknown>");
}

/* Accoring to section 9.3.5 of the UM, the AVIC doesn't accelerate
 * fast interrupts and they must be dispatched */
void __attribute__((naked)) fiq_handler(void)
{
    asm volatile (
        "mov r10, #0x68000000      \n" /* load AVIC base address */
        "ldr r9, [r10, #0x44]      \n" /* read FIVECSR of AVIC */
        "add r10, r10, #0x100      \n" /* move pointer to base of VECTOR table */
        "ldr r8, [r10, r9, lsl #2] \n" /* read FIQ vector from VECTOR table */
        "bx  r8                    \n" /* jump to FIQ service routine */
    );
}

void avic_init(void)
{
    struct avic_map * const avic = (struct avic_map *)AVIC_BASE_ADDR;
    int i;

    /* Disable all interrupts and set to unhandled */
    avic_disable_int(ALL);

    /* Reset AVIC control */
    avic->intcntl = 0;

    /* Init all interrupts to type IRQ */
    avic_set_int_type(ALL, IRQ);

    /* Set all normal to lowest priority */
    for (i = 0; i < 8; i++)
        avic->nipriority[i] = 0;

    /* Set NM bit to enable VIC */
    avic->intcntl |= INTCNTL_NM;

    /* Enable VE bit in CP15 Control reg to enable VIC */
    asm volatile (
        "mrc p15, 0, r0, c1, c0, 0 \n"
        "orr r0, r0, #(1 << 24)    \n"
        "mcr p15, 0, r0, c1, c0, 0 \n"
        : : : "r0");

    /* Enable normal interrupts at all priorities */
    avic->nimask = 0x1f;
}

void avic_set_int_priority(enum IMX31_INT_LIST ints,
                           unsigned long ni_priority)
{
    struct avic_map * const avic = (struct avic_map *)AVIC_BASE_ADDR;
    volatile uint32_t *reg = &avic->nipriority[7 - (ints >> 3)];
    unsigned int shift = (ints & 0x7) << 2;
    uint32_t mask = 0xful << shift;
    *reg = (*reg & ~mask) | ((ni_priority << shift) & mask);
}

void avic_enable_int(enum IMX31_INT_LIST ints, enum INT_TYPE intstype,
                     unsigned long ni_priority, void (*handler)(void))
{
    struct avic_map * const avic = (struct avic_map *)AVIC_BASE_ADDR;
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);

    if (ints != ALL) /* No mass-enable allowed */
    {
        avic_set_int_type(ints, intstype);
        avic->vector[ints] = (long)handler;
        avic->intennum = ints;
        avic_set_int_priority(ints, ni_priority);
    }

    restore_interrupt(oldstatus);
}

void avic_disable_int(enum IMX31_INT_LIST ints)
{
    struct avic_map * const avic = (struct avic_map *)AVIC_BASE_ADDR;
    uint32_t i;

    if (ints == ALL)
    {
        for (i = 0; i < 64; i++)
        {
            avic->intdisnum = i;
            avic->vector[i] = (long)UIE_VECTOR;
        }
    }
    else
    {
        avic->intdisnum = ints;
        avic->vector[ints] = (long)UIE_VECTOR;
    }
}

static void set_int_type(int i, enum INT_TYPE intstype)
{
    /* INTTYPEH: vectors 63-32, INTTYPEL: vectors 31-0 */
    struct avic_map * const avic = (struct avic_map *)AVIC_BASE_ADDR;
    volatile uint32_t *reg = &avic->inttype[1 - (i >> 5)];
    uint32_t val = 1L << (i & 0x1f);

    if (intstype == IRQ)
        val = *reg & ~val;
    else
        val = *reg | val;

    *reg = val;
}

void avic_set_int_type(enum IMX31_INT_LIST ints, enum INT_TYPE intstype)
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);

    if (ints == ALL)
    {
        int i;
        for (i = 0; i < 64; i++)
            set_int_type(i, intstype);
    }
    else
    {
        set_int_type(ints, intstype);
    }

    restore_interrupt(oldstatus);
}
