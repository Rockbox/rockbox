/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2013 by Marcin Bukat
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
#include "rk27xx.h"

/**
 *
 * Global
 *
 */

enum rk27xx_family_t
{
    UNKNOWN,
    REV_A,
    REV_B,
};

static enum rk27xx_family_t g_rk27xx_family = UNKNOWN;
static int g_atexit = HWSTUB_ATEXIT_OFF;

static void power_off(void)
{
    GPIO_PCCON &= ~(1<<0);
    while(1);
}

static void rk27xx_reset(void)
{
    /* use Watchdog to reset */
    SCU_CLKCFG &= ~CLKCFG_WDT;
    WDTLR = 1;
    WDTCON = (1<<4) | (1<<3);

    /* Wait for reboot to kick in */
    while(1);
}

#define HZ  1000000

static void backlight_init(void)
{
    /* configure PD4 as output */
    GPIO_PDCON |= (1<<4);

    /* set PD4 low (backlight off) */
    GPIO_PDDR &= ~(1<<4);

    /* IOMUXB - set PWM0 pin as GPIO */
    SCU_IOMUXB_CON &= ~(1 << 11); /* type<<11<<channel */

    /* DIV/2, PWM reset */
    PWMT0_CTRL = (0<<9) | (1<<7);

    /* set pwm frequency */
    /* (apb_freq/pwm_freq)/pwm_div = (50 000 000/pwm_freq)/2 */
    PWMT0_LRC = 50000;
    PWMT0_HRC = 50000;

    /* reset counter */
    PWMT0_CNTR = 0x00;

    /* DIV/2, PWM output enable, PWM timer enable */
    PWMT0_CTRL = (0<<9) | (1<<3) | (1<<0);
}

void backlight_on(void)
{
    /* enable PWM clock */
    SCU_CLKCFG &= ~CLKCFG_PWM;

    /* set output pin as PWM pin */
    SCU_IOMUXB_CON |= (1<<11); /* type<<11<<channel */

    /* pwm enable */
    PWMT0_CTRL |= (1<<3) | (1<<0);
}

void backlight_off(void)
{
    /* setup PWM0 pin as GPIO which is pulled low */
    SCU_IOMUXB_CON &= ~(1<<11);

    /* stop pwm timer */
    PWMT0_CTRL &= ~(1<<3) | (1<<0);

    /* disable PWM clock */
    SCU_CLKCFG |= CLKCFG_PWM;
}

static void _enable_irq(void)
{
    asm volatile ("mrs r0, cpsr\n"
                  "bic r0, r0, #0x80\n"
                  "msr cpsr_c, r0\n"
                 );
}

static void _disable_irq(void)
{
    asm volatile ("mrs r0, cpsr\n"
                  "orr r0, r0, #0x80\n"
                  "msr cpsr_c, r0\n"
                 );
}

void target_init(void)
{
    /* ungate all clocks */
    SCU_CLKCFG = 0;

    /* keep act line */
    GPIO_PCDR |= (1<<0);
    GPIO_PCCON |= (1<<0);

    /* disable watchdog */
    WDTCON &= ~(1<<3);

    /* enable UDC interrupt */
    INTC_IMR = (1<<16);
    INTC_IECR = (1<<16);

    EN_INT = EN_SUSP_INTR   |  /* Enable Suspend Interrupt */
             EN_RESUME_INTR |  /* Enable Resume Interrupt */
             EN_USBRST_INTR |  /* Enable USB Reset Interrupt */
             EN_OUT0_INTR   |  /* Enable OUT Token receive Interrupt EP0 */
             EN_IN0_INTR    |  /* Enable IN Token transmits Interrupt EP0 */
             EN_SETUP_INTR;    /* Enable SETUP Packet Receive Interrupt */

    /* 6. configure INTCON */
    INTCON = UDC_INTHIGH_ACT |  /* interrupt high active */
             UDC_INTEN;         /* enable EP0 interrupts */

    /* enable irq */
    _enable_irq();

    backlight_init();
    backlight_on();

    /* detect revision */
    uint32_t rk27xx_id = SCU_ID;
    if(rk27xx_id == 0xa1000604)
    {
        logf("identified rk27xx REV_A \n");
        g_rk27xx_family = REV_A;
    }
    else if(rk27xx_id == 0xa10002b7)
    {
        logf("identified rk27xx REV_B \n");
        g_rk27xx_family = REV_B;
    }
    else
    {
        logf("unknown rk27xx revision \n");
    }
}

static struct usb_resp_info_target_t g_target =
{
    .id = HWSTUB_TARGET_RK27,
    .name = "Rockchip RK27XX"
};

int target_get_info(int info, void **buffer)
{
#if 0
    if(info == HWSTUB_INFO_STMP)
    {
        g_stmp.chipid = __XTRACT(HW_DIGCTL_CHIPID, PRODUCT_CODE);
        g_stmp.rev = __XTRACT(HW_DIGCTL_CHIPID, REVISION);
        g_stmp.is_supported = g_stmp_family != 0;
        *buffer = &g_stmp;
        return sizeof(g_stmp);
    }
#endif
    if(info == HWSTUB_INFO_TARGET)
    {
        *buffer = &g_target;
        return sizeof(g_target);
    }
    else
        return -1;
}

int target_atexit(int method)
{
    g_atexit = method;
    return 0;
}

void target_exit(void)
{
    switch(g_atexit)
    {
        case HWSTUB_ATEXIT_OFF:
            power_off();
            // fallthrough in case of return
        case HWSTUB_ATEXIT_REBOOT:
            rk27xx_reset();
            // fallthrough in case of return
        case HWSTUB_ATEXIT_NOP:
        default:
            return;
    }
}
