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

#include "regs/tcu.h"
#include "regs/ost.h"
#include "regs/gpio.h"
#include "regs/cpm.h"

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

void jz_ost_init(void)
{
    /* Assume EXTCLK is 12MHz, use no prescaler so that OS timer is running at 12Mhz */
    jz_clrf(TCU_ENABLE, OST);
    /* modification must be done with OST disabled */
    jz_overwritef(OST_CTRL, IGNORE_COMPARE(1), PRESCALE_V(1), EXT_EN(1));
    jz_write(OST_COUNTL, 0);
    jz_write(OST_COUNTH, 0);
    jz_setf(TCU_ENABLE, OST);
}

uint64_t jz_ost_read64(void)
{
    /* high part is buffered when low is read */
    uint32_t low = jz_read(OST_COUNTL);
    return low /*| (uint64_t)jz_read(OST_COUNTH_BUF) << 32*/;
}

void udelay(unsigned int usec)
{
    /* TCU init code setups OS timer at 12Mhz */
    uint64_t end = jz_ost_read64() + 12 * usec;
    while(jz_ost_read64() < end) {}
}

void system_reboot(void)
{
    while (1);
}

void system_exception_wait(void)
{
    /* check for power button without including any .h file */
    while(1) {}
}

void power_off(void)
{
    while(1);
}

static inline void jz_gpio_set_drive(unsigned bank, unsigned pin, unsigned strength)
{
}

static inline void jz_gpio_set_output(unsigned bank, unsigned pin, bool value)
{
    if(value)
        jz_set(GPIO_OUT(bank), 1 << pin);
    else
        jz_clr(GPIO_OUT(bank), 1 << pin);
}

static inline bool jz_gpio_get_input(unsigned bank, unsigned pin)
{
    return (jz_read(GPIO_IN(bank)) >> pin) & 1;
}

#define __PIN_FUNCTION  (1 << 0)
#define __PIN_SELECT    (1 << 1)
#define __PIN_DIR       (1 << 2)
#define __PIN_TRIGGER   (1 << 3)

#define PIN_GPIO_OUT    __PIN_DIR
#define PIN_GPIO_IN     0

static inline void jz_gpio_set_function(unsigned bank, unsigned pin, unsigned function)
{
    /* NOTE the encoding of function gives the proper values to write */
    if(function & __PIN_FUNCTION)
        jz_set(GPIO_FUNCTION(bank), 1 << pin);
    else
        jz_clr(GPIO_FUNCTION(bank), 1 << pin);
    if(function & __PIN_SELECT)
        jz_set(GPIO_SELECT(bank), 1 << pin);
    else
        jz_clr(GPIO_SELECT(bank), 1 << pin);
    if(function & __PIN_DIR)
        jz_set(GPIO_DIR(bank), 1 << pin);
    else
        jz_clr(GPIO_DIR(bank), 1 << pin);
    if(function & __PIN_TRIGGER)
        jz_set(GPIO_TRIGGER(bank), 1 << pin);
    else
        jz_clr(GPIO_TRIGGER(bank), 1 << pin);
}

static inline void jz_gpio_enable_pullup(unsigned bank, unsigned pin, bool enable)
{
    /* warning: register meaning is "disable pullup" */
    if(enable)
        jz_clr(GPIO_PULL(bank), 1 << pin);
    else
        jz_set(GPIO_PULL(bank), 1 << pin);
}

/* memory init, called very early at boot */
void memory_init(void)
{
    /* reset mmu to a known state */
    mmu_init();
    /* ungated AHB1 so that TCSM0 access work through physical address */
    jz_writef(CPM_CLKGATE1, AHB1(0));
    /* map TCSM0 after DRAM (see jz4760b.h for explanation) */
    map_address(IRAM_ORIG, PHYSICAL_IRAM_ADDR, IRAM_SIZE, K_CacheAttrU);
}

void system_init(void)
{
    jz_ost_init();
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
