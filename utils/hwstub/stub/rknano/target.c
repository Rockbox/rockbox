/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Amaury Pouly
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
#include "stddef.h"
#include "target.h"
#include "system.h"
#include "logf.h"
#include "usb_drv.h"

#define NVIC_ISER0  (*(volatile uint32_t *)0xe000e100)
#define NVIC_ICER0  (*(volatile uint32_t *)0xe000e180)
#define NVIC_ICPR0  (*(volatile uint32_t *)0xe000e280)

#define SYST_CSR    (*(volatile uint32_t *)0xe000e010)
#define SYST_CSR_ENABLE     (1 << 0)
#define SYST_CSR_COUNTFLAG  (1 << 16)
#define SYST_RVR    (*(volatile uint32_t *)0xe000e014)
#define SYST_RVR_RELOAD_BP  0
#define SYST_RVR_RELOAD_BM  (0xffffff << 0)
#define SYST_CVR    (*(volatile uint32_t *)0xe000e018)
#define SYST_CVR_CURRENT_BP 0
#define SYST_CVR_CURRENT_BM 0xffffffff
#define SYST_CALIB  (*(volatile uint32_t *)0xe000e01c)
#define SYST_CALIB_TENMS_BP 0
#define SYST_CALIB_TENMS_BM (0xffffff << 0)

#define FIELD_MAKE(reg,field,val) \
    (((val) << reg##_##field##_BP) & reg##_##field##_BM)
#define FIELD_SET(reg,field,val) \
    reg = (reg & ~(reg##_##field##_BM)) | (((val) << reg##_##field##_BP) & reg##_##field##_BM)
#define FIELD_GET(reg,field) \
    ((reg & reg##_##field##_BM) >> reg##_##field##_BP)

#define implement_extint(i) \
    void __attribute__((interrupt,weak)) EXTINT##i(void) {}

implement_extint(0)
implement_extint(1)
implement_extint(2)
implement_extint(3)
implement_extint(4)
implement_extint(5)
implement_extint(6)
implement_extint(8)
implement_extint(9)
implement_extint(10)
implement_extint(11)
implement_extint(12)
implement_extint(13)
implement_extint(14)
implement_extint(15)
implement_extint(16)
implement_extint(17)
implement_extint(18)

/**
 *
 * Global
 *
 */

/**
 *
 * Power
 *
 */

void power_off(void)
{
}

void reset(void)
{
}

void target_init(void)
{
    /* disable all interrupt sources */
    NVIC_ICER0 = 0xffffffff;
    NVIC_ICPR0 = 0xffffffff;
    /* enable interrupts */
    __asm volatile("cpsie i\n");
    SYST_CSR |= SYST_CSR_ENABLE;
    SYST_RVR = SYST_RVR_RELOAD_BM; // maximum value
}

struct hwstub_target_desc_t __attribute__((aligned(2))) target_descriptor =
{
    sizeof(struct hwstub_target_desc_t),
    HWSTUB_DT_TARGET,
    HWSTUB_TARGET_RKNANO,
    "RKNano-B/C"
};

void target_get_desc(int desc, void **buffer)
{
    (void) desc;
    *buffer = NULL;
}

void target_get_config_desc(void *buffer, int *size)
{
    (void) buffer;
    (void) size;
}

void target_udelay(int us)
{
#if 1
    /* CPU runs at ~29Minstr/s in DFU mode
     * and the loop has 3 instr/op */
    us *= 10;
    asm volatile(
        "1: cmp %[x], #0\n"
        "   sub %[x], #1\n"
        "   bne 1b\n"
        :[x]"+l"(us));
#else
    uint32_t tenms = FIELD_GET(SYST_CALIB, TENMS);
    uint32_t start = FIELD_GET(SYST_CVR, CURRENT);
    uint64_t tmp = (uint64_t)us * (uint64_t)tenms;
    uint32_t delay = /*(us * tenms) / 10000*/ ((uint32_t)(tmp >> 32)) * 429496 + (uint32_t)tmp / 10000;
    uint32_t end;
    if(start >= delay)
        end = start - delay;
    else
        end = FIELD_GET(SYST_RVR, RELOAD) - (delay - start);
    while(1)
    {
        uint32_t cur = FIELD_GET(SYST_CVR, CURRENT);
        if(start >= delay && (cur < end || cur > start))
            break;
        if(start < delay && (cur < end && cur > start))
            break;
    }
#endif
}

void target_mdelay(int ms)
{
    while(ms >= 5)
    {
        target_udelay(5000);
        ms -= 5;
    }
    target_udelay(ms * 1000);
}

void target_enable_usb_clocks(void)
{
}

void target_enable_usb_irq(void)
{
    NVIC_ISER0 = 0x80;
}

void target_disable_usb_irq(void)
{
    NVIC_ICER0 = 0x80;
}

void target_clear_usb_irq(void)
{
    NVIC_ICPR0 = 0x80;
}

void invalidate_dcache(const void* addr, uint32_t len)
{
    (void) addr;
    (void) len;
}

void clean_dcache(const void* addr, uint32_t len)
{
    (void) addr;
    (void) len;
}

void __attribute__((interrupt)) EXTINT7(void)
{
    usb_drv_irq();
}
