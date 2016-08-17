/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 by Amaury Pouly
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
#include "jz4760b.h"
#include "mips.h"
#include "panic.h"
#include "system.h"
#include "kernel.h"
#include "power.h"

#include "jz4760b_regs.h"
#include "gpio-jz4760b.h"

static void UIRQ(void)
{
    panicf("Unhandled interrupt occurred");
}

void intr_handler(void)
{
    while(1) {}
}

#define EXC(x,y) case (x): return (y);
static char* parse_exception(unsigned int cause)
{
    switch(cause & M_CauseExcCode)
    {
        EXC(EXC_INT, "Interrupt");
        EXC(EXC_MOD, "TLB Modified");
        EXC(EXC_TLBL, "TLB Exception (Load or Ifetch)");
        EXC(EXC_ADEL, "Address Error (Load or Ifetch)");
        EXC(EXC_ADES, "Address Error (Store)");
        EXC(EXC_TLBS, "TLB Exception (Store)");
        EXC(EXC_IBE, "Instruction Bus Error");
        EXC(EXC_DBE, "Data Bus Error");
        EXC(EXC_SYS, "Syscall");
        EXC(EXC_BP, "Breakpoint");
        EXC(EXC_RI, "Reserved Instruction");
        EXC(EXC_CPU, "Coprocessor Unusable");
        EXC(EXC_OV, "Overflow");
        EXC(EXC_TR, "Trap Instruction");
        EXC(EXC_FPE, "Floating Point Exception");
        EXC(EXC_C2E, "COP2 Exception");
        EXC(EXC_MDMX, "MDMX Exception");
        EXC(EXC_WATCH, "Watch Exception");
        EXC(EXC_MCHECK, "Machine Check Exception");
        EXC(EXC_CacheErr, "Cache error caused re-entry to Debug Mode");
        default:
            return NULL;
    }
}
#undef EXC

void exception_handler(void* stack_ptr, unsigned int cause, unsigned int epc)
{
    panicf("Exception occurred: %s [0x%08x] at 0x%08x (stack at 0x%08x)", parse_exception(cause), read_c0_badvaddr(), epc, (unsigned int)stack_ptr);
}

void tlb_refill_handler(void)
{
    panicf("TLB refill handler at 0x%08lx! [0x%x]", read_c0_epc(), read_c0_badvaddr());
}

static void jz_ost_init(void)
{
    /* Assume EXTCLK is 12MHz, use no prescaler so that OS timer is running at 12Mhz */
    TCU_ENABLE_CLR = TCU_ENABLE_OST;
    /* modification must be done with OST disabled */
    OST_CTRL = OST_CTRL_IGNORE_COMPARE | OST_CTRL_PRESCALE_v(1) | OST_CTRL_EXT_EN;
    OST_COUNTL = 0;
    OST_COUNTH = 0;
    TCU_ENABLE_SET = TCU_ENABLE_OST;
}

uint64_t jz_ost_read64(void)
{
    /* high part is buffered when low is read */
    uint32_t low = OST_COUNTL;
    return low | (uint64_t)OST_COUNTH_BUF << 32;
}

void udelay(unsigned int usec)
{
    /* OS timer is running at 12Mhz */
    uint64_t end = jz_ost_read64() + 12 * usec + 1;
    while(jz_ost_read64() < end) {}
}

void system_reboot(void)
{
    while(1) {}
}

void system_exception_wait(void)
{
    /* check for power button without including any .h file */
    while(1) {}
}

void power_off(void)
{
    while(1) {}
}

/* memory init, called very early at boot */
void memory_init(void)
{
    /* reset mmu to a known state */
    mmu_init();
    /* ungated AHB1 so that TCSM0 access work through physical address */
    CPM_CLKGATE1 &= ~CPM_CLKGATE1_AHB1;
    /* map TCSM0 after DRAM (see jz4760b.h for explanation) */
    map_address(IRAM_ORIG, PHYSICAL_IRAM_ADDR, IRAM_SIZE, K_CacheAttrU);
}

void system_init(void)
{
    jz_ost_init();
}

void die_blink(void)
{
    jz_gpio_set_function(4, 1, PIN_GPIO_OUT);
    jz_gpio_enable_pullup(4, 1, false);
    while(1)
    {
        jz_gpio_set_output(4, 1, true);
        mdelay(250);
        jz_gpio_set_output(4, 1, false);
        mdelay(250);
    }
}

int system_memory_guard(int newmode)
{
    (void)newmode;
    return 0;
}

#ifdef HAVE_ADJUSTABLE_CPU_FREQ
void set_cpu_frequency(long frequency)
{

}
#endif
