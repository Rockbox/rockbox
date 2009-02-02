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

#include "kernel.h"
#include "system.h"
#include "panic.h"
#include "avic-imx31.h"
#include "gpio-imx31.h"
#include "mmu-imx31.h"
#include "system-target.h"
#include "lcd.h"
#include "serial-imx31.h"
#include "debug.h"
#include "clkctl-imx31.h"
#include "mc13783.h"

/** Watchdog timer routines **/

/* Initialize the watchdog timer */
void watchdog_init(unsigned int half_seconds)
{
    uint16_t wcr = WDOG_WCR_WTw(half_seconds) | /* Timeout */
                   WDOG_WCR_WOE |       /* WDOG output enabled */
                   WDOG_WCR_WDA |       /* WDOG assertion - no effect */
                   WDOG_WCR_SRS |       /* System reset - no effect */
                   WDOG_WCR_WRE;        /* Generate a WDOG signal */

    imx31_clkctl_module_clock_gating(CG_WDOG, CGM_ON_RUN_WAIT);

    WDOG_WCR = wcr;
    WDOG_WSR = 0x5555;
    WDOG_WCR = wcr | WDOG_WCR_WDE; /* Enable timer - hardware does
                                      not allow a disable now */
    WDOG_WSR = 0xaaaa;
}

/* Service the watchdog timer */
void watchdog_service(void)
{
    WDOG_WSR = 0x5555;
    WDOG_WSR = 0xaaaa;
}

/** GPT timer routines - basis for udelay **/

/* Start the general-purpose timer (1MHz) */
void gpt_start(void)
{
    imx31_clkctl_module_clock_gating(CG_GPT, CGM_ON_RUN_WAIT);
    unsigned int ipg_mhz = imx31_clkctl_get_ipg_clk() / 1000000;

    GPTCR &= ~GPTCR_EN; /* Disable counter */
    GPTCR |= GPTCR_SWR; /* Reset module */
    while (GPTCR & GPTCR_SWR);
    /* No output
     * No capture
     * Enable in run mode only (doesn't tick while in WFI)
     * Freerun mode (count to 0xFFFFFFFF and roll-over to 0x00000000)
     */
    GPTCR = GPTCR_FRR | GPTCR_CLKSRC_IPG_CLK;
    GPTPR = ipg_mhz - 1;
    GPTCR |= GPTCR_EN;
}

/* Stop the general-purpose timer */
void gpt_stop(void)
{
    GPTCR &= ~GPTCR_EN;
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

void system_reboot(void)
{
    /* Multi-context so no SPI available (WDT?)  */
    while (1);
}

void system_exception_wait(void)
{
    /* Called in many contexts so button reading may be a chore */
    avic_disable_int(ALL);
    core_idle();
    while (1);
}

void system_init(void)
{
    static const int disable_clocks[] =
    {
        /* CGR0 */
        CG_SD_MMC1,
        CG_SD_MMC2,
        CG_IIM,
        CG_SDMA,
        CG_CSPI3,
        CG_RNG,
        CG_UART1,
        CG_UART2,
        CG_SSI1,
        CG_I2C1,
        CG_I2C2,
        CG_I2C3,

        /* CGR1 */
        CG_HANTRO,
        CG_MEMSTICK1,
        CG_MEMSTICK2,
        CG_CSI,
        CG_RTC,
        CG_WDOG,
        CG_PWM,
        CG_SIM,
        CG_ECT,
        CG_USBOTG,
        CG_KPP,
        CG_UART3,
        CG_UART4,
        CG_UART5,
        CG_1_WIRE,

        /* CGR2 */
        CG_SSI2,
        CG_CSPI1,
        CG_CSPI2,
        CG_GACC,
        CG_RTIC,
        CG_FIR
    };

    unsigned int i;

    /* MCR WFI enables wait mode */
    CLKCTL_CCMR &= ~(3 << 14);

    imx31_regset32(&SDHC1_CLOCK_CONTROL, STOP_CLK);
    imx31_regset32(&SDHC2_CLOCK_CONTROL, STOP_CLK);
    imx31_regset32(&RNGA_CONTROL, RNGA_CONTROL_SLEEP);
    imx31_regclr32(&UCR1_1, EUARTUCR1_UARTEN);
    imx31_regclr32(&UCR1_2, EUARTUCR1_UARTEN);
    imx31_regclr32(&UCR1_3, EUARTUCR1_UARTEN);
    imx31_regclr32(&UCR1_4, EUARTUCR1_UARTEN);
    imx31_regclr32(&UCR1_5, EUARTUCR1_UARTEN);

    for (i = 0; i < ARRAYLEN(disable_clocks); i++)
        imx31_clkctl_module_clock_gating(disable_clocks[i], CGM_OFF);

    avic_init();
    gpt_start();
    gpio_init();
}

void  __attribute__((naked)) imx31_regmod32(volatile uint32_t *reg_p,
                                            uint32_t value,
                                            uint32_t mask)
{
    asm volatile("and    r1, r1, r2 \n"
                 "mrs    ip, cpsr   \n"
                 "cpsid  if         \n"
                 "ldr    r3, [r0]   \n"
                 "bic    r3, r3, r2 \n" 
                 "orr    r3, r3, r1 \n"
                 "str    r3, [r0]   \n"
                 "msr    cpsr_c, ip \n"
                 "bx     lr         \n");
    (void)reg_p; (void)value; (void)mask;
}

void __attribute__((naked)) imx31_regset32(volatile uint32_t *reg_p,
                                           uint32_t mask)
{
    asm volatile("mrs    r3, cpsr   \n"
                 "cpsid  if         \n"
                 "ldr    r2, [r0]   \n"
                 "orr    r2, r2, r1 \n"
                 "str    r2, [r0]   \n"
                 "msr    cpsr_c, r3 \n"
                 "bx     lr         \n");
    (void)reg_p; (void)mask;
}

void __attribute__((naked)) imx31_regclr32(volatile uint32_t *reg_p,
                                           uint32_t mask)
{
    asm volatile("mrs    r3, cpsr   \n"
                 "cpsid  if         \n"
                 "ldr    r2, [r0]   \n"
                 "bic    r2, r2, r1 \n"
                 "str    r2, [r0]   \n"
                 "msr    cpsr_c, r3 \n"
                 "bx     lr         \n");
    (void)reg_p; (void)mask;
}

#ifdef BOOTLOADER
void system_prepare_fw_start(void)
{
    disable_interrupt(IRQ_FIQ_STATUS);
    avic_disable_int(ALL);
    mc13783_close();
    tick_stop();
}
#endif

inline void dumpregs(void) 
{
	asm volatile ("mov %0,r0\n\t"
				  "mov %1,r1\n\t"
				  "mov %2,r2\n\t"
				  "mov %3,r3":
				  "=r"(regs.r0),"=r"(regs.r1),
				  "=r"(regs.r2),"=r"(regs.r3):);

    asm volatile ("mov %0,r4\n\t"
				  "mov %1,r5\n\t"
				  "mov %2,r6\n\t"
				  "mov %3,r7":
				  "=r"(regs.r4),"=r"(regs.r5),
				  "=r"(regs.r6),"=r"(regs.r7):);

    asm volatile ("mov %0,r8\n\t"
				  "mov %1,r9\n\t"
				  "mov %2,r10\n\t"
				  "mov %3,r12":
				  "=r"(regs.r8),"=r"(regs.r9),
				  "=r"(regs.r10),"=r"(regs.r11):);
  
	asm volatile ("mov %0,r12\n\t"
				   "mov %1,sp\n\t"
				   "mov %2,lr\n\t"
				   "mov %3,pc\n"
				   "sub %3,%3,#8":
				   "=r"(regs.r12),"=r"(regs.sp),
				   "=r"(regs.lr),"=r"(regs.pc):);
#ifdef HAVE_SERIAL
    dprintf("Register Dump :\n");
	dprintf("R0=0x%x\tR1=0x%x\tR2=0x%x\tR3=0x%x\n",regs.r0,regs.r1,regs.r2,regs.r3);
    dprintf("R4=0x%x\tR5=0x%x\tR6=0x%x\tR7=0x%x\n",regs.r4,regs.r5,regs.r6,regs.r7);
	dprintf("R8=0x%x\tR9=0x%x\tR10=0x%x\tR11=0x%x\n",regs.r8,regs.r9,regs.r10,regs.r11);
	dprintf("R12=0x%x\tSP=0x%x\tLR=0x%x\tPC=0x%x\n",regs.r12,regs.sp,regs.lr,regs.pc);
	//dprintf("CPSR=0x%x\t\n",regs.cpsr);
#endif
	DEBUGF("Register Dump :\n");
	DEBUGF("R0=0x%x\tR1=0x%x\tR2=0x%x\tR3=0x%x\n",regs.r0,regs.r1,regs.r2,regs.r3);
    DEBUGF("R4=0x%x\tR5=0x%x\tR6=0x%x\tR7=0x%x\n",regs.r4,regs.r5,regs.r6,regs.r7);
	DEBUGF("R8=0x%x\tR9=0x%x\tR10=0x%x\tR11=0x%x\n",regs.r8,regs.r9,regs.r10,regs.r11);
	DEBUGF("R12=0x%x\tSP=0x%x\tLR=0x%x\tPC=0x%x\n",regs.r12,regs.sp,regs.lr,regs.pc);
	//DEBUGF("CPSR=0x%x\t\n",regs.cpsr);
    
 }

#ifdef HAVE_ADJUSTABLE_CPU_FREQ

void set_cpu_frequency(long frequency)
{
    (void)freqency;
}

#endif
