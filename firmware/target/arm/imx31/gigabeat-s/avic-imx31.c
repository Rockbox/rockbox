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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
    long offset;

    asm volatile (
        "mrs    %0, cpsr      \n" /* Mask core IRQ/FIQ */
        "orr    %0, %0, #0xc0 \n"
        "msr    cpsr_c, %0    \n"
        "and    %0, %0, #0x1f \n" /* Get mode bits */
        : "=&r"(mode)
    );

    offset = mode == 0x11 ? (long)FIVECSR : ((long)NIVECSR >> 16);

    panicf("Unhandled %s %ld: %s",
           mode == 0x11 ? "FIQ" : "IRQ", offset,
           offset >= 0 ? avic_int_names[offset] : "<Unknown>");
}

/* We use the AVIC */
void __attribute__((naked)) irq_handler(void)
{
    panicf("Unhandled IRQ in irq_handler");
}

/* Accoring to section 9.3.5 of the UM, the AVIC doesn't accelerate
 * fast interrupts and they must be dispatched */
void __attribute__((naked)) fiq_handler(void)
{
    asm volatile (
        "mov r10, #0x6c000000      \n" /* load AVIC base address */
        "ldr r9, [r10, #0x44]      \n" /* read FIVECSR of AVIC */
        "add r10, r10, #100        \n" /* move pointer to base of VECTOR table */
        "ldr r8, [r10, r9, lsl #2] \n" /* read FIQ vector from VECTOR table */
        "bx  r8                    \n" /* jump to FIQ service routine */
    );
}

void avic_init(void)
{
    int i;

    /* Disable all interrupts and set to unhandled */
    avic_disable_int(ALL);

    /* Reset AVIC control */
    INTCNTL = 0;

    /* Init all interrupts to type IRQ */
    avic_set_int_type(ALL, IRQ);

    /* Set all normal to lowest priority */
    for (i = 0; i < 8; i++)
        NIPRIORITY(i) = 0;

    /* Set NM bit to enable VIC */
    INTCNTL |= INTCNTL_NM;

    /* Enable VE bit in CP15 Control reg to enable VIC */
    asm volatile (
        "mrc p15, 0, r0, c1, c0, 0 \n"
        "orr r0, r0, #(1 << 24)    \n"
        "mcr p15, 0, r0, c1, c0, 0 \n"
        : : : "r0");

    /* Enable normal interrupts at all priorities */
    NIMASK = 0x1f;
}

void avic_set_int_priority(enum IMX31_INT_LIST ints,
                           unsigned long ni_priority)
{
    volatile unsigned long *reg = &NIPRIORITY((63 - ints) / 8);
    unsigned int shift = 4*(ints % 8);
    unsigned long mask = 0xful << shift;
    *reg = (*reg & ~mask) | ((ni_priority << shift) & mask);
}

void avic_enable_int(enum IMX31_INT_LIST ints, enum INT_TYPE intstype,
                     unsigned long ni_priority, void (*handler)(void))
{
    int oldstatus = disable_interrupt_save(IRQ_FIQ_STATUS);

    if (ints != ALL) /* No mass-enable allowed */
    {
        avic_set_int_type(ints, intstype);
        VECTOR(ints) = (long)handler;
        INTENNUM = ints;
        avic_set_int_priority(ints, ni_priority);
    }

    restore_interrupt(oldstatus);
}

void avic_disable_int(enum IMX31_INT_LIST ints)
{
    long i;

    if (ints == ALL)
    {
        for (i = 0; i < 64; i++)
        {
            INTDISNUM = i;
            VECTOR(i) = (long)UIE_VECTOR;
        }
    }
    else
    {
        INTDISNUM = ints;
        VECTOR(ints) = (long)UIE_VECTOR;
    }
}

static void set_int_type(int i, enum INT_TYPE intstype)
{
    volatile unsigned long *reg;
    long val;

    if (i >= 32)
    {
        reg = &INTTYPEH;
        val = 1L << (i - 32);
    }
    else
    {
        reg = &INTTYPEL;
        val = 1L << i;
    }

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
