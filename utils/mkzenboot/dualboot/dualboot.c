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
#include "regs-pinctrl.h"
#include "regs-power.h"
#include "regs-lradc.h"
#include "regs-digctl.h"

typedef unsigned long uint32_t;

// target specific boot decision
enum boot_t
{
    BOOT_STOP,  /* power down */
    BOOT_ROCK, /* boot to Rockbox */
    BOOT_OF, /* boot to OF */
};

/**
 * Helper functions
 */

static inline int __attribute__((always_inline)) read_gpio(int bank, int pin)
{
    return (HW_PINCTRL_DINn(bank) >> pin) & 1;
}

static inline int __attribute__((always_inline)) read_pswitch(void)
{
#if IMX233_SUBTARGET >= 3700
    return BF_RD(POWER_STS, PSWITCH);
#else
    return BF_RD(DIGCTL_STATUS, PSWITCH);
#endif
}

/* only works for channels <=7, always divide by 2, never accumulates */
static inline void __attribute__((always_inline)) setup_lradc(int src)
{
    BF_CLR(LRADC_CTRL0, SFTRST);
    BF_CLR(LRADC_CTRL0, CLKGATE);
    /* don't bother changing the source, we are early enough at boot so that
     * channel x is mapped to source x */
    HW_LRADC_CHn_CLR(src) = BM_OR2(LRADC_CHn, NUM_SAMPLES, ACCUMULATE);
    BF_SETV(LRADC_CTRL2, DIVIDE_BY_TWO, 1 << src);
}

#define BP_LRADC_CTRL1_LRADCx_IRQ(x)    (x)
#define BM_LRADC_CTRL1_LRADCx_IRQ(x)    (1 << (x))

static inline int __attribute__((always_inline)) read_lradc(int src)
{
    BF_CLR(LRADC_CTRL1, LRADCx_IRQ(src));
    BF_SETV(LRADC_CTRL0, SCHEDULE, 1 << src);
    while(!BF_RD(LRADC_CTRL1, LRADCx_IRQ(src)));
    return BF_RDn(LRADC_CHn, src, VALUE);
}

static inline void __attribute__((noreturn)) power_down()
{
    /* power down */
    HW_POWER_RESET = BM_OR2(POWER_RESET, UNLOCK, PWD);
    while(1);
}

/**
 * Boot decision functions
 */

#if defined(CREATIVE_ZENMOZAIC) || defined(CREATIVE_ZEN) || defined(CREATIVE_ZENXFI) \
    || defined(CREATIVE_ZENV)
static enum boot_t boot_decision()
{
    setup_lradc(0); // setup LRADC channel 0 to read keys
    /* make a decision */
    /* read keys */
    int val = read_lradc(0);
    /* if back is pressed, boot to OF
     * otherwise boot to RB */
    if(val >= 2650 && val < 2750) // conveniently, all players use the same value
        return BOOT_OF;
    return BOOT_ROCK;
}
#else
#warning You should define a target specific boot decision function
static int boot_decision()
{
    return BOOT_ROCK;
}
#endif

static int main(uint32_t rb_addr, uint32_t of_addr)
{
    switch(boot_decision())
    {
        case BOOT_ROCK:
            return rb_addr;
        case BOOT_OF:
            /* fix back the loading address
            /* NOTE: see mkzenboot for more details */
            *(uint32_t *)0x20 = of_addr;
            return 0;
        case BOOT_STOP:
        default:
            power_down();
    }
}

/** Glue for the linker mostly */

extern uint32_t of_vector;
extern uint32_t rb_vector;
extern uint32_t boot_arg;

void __attribute__((section(".start"))) start()
{
    uint32_t addr = main(rb_vector, of_vector);
    ((void (*)(uint32_t))addr)(boot_arg);
}
