/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
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
#include "cpu.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "pcf50606.h"
#include "pcf50635.h"
#include "button-target.h"
#include "touchscreen-target.h"
#include "tuner.h"
#include "backlight-target.h"
#include "powermgmt.h"
#include "power-target.h"

static enum pmu_type pmu;

enum pmu_type get_pmu_type()
{
    return pmu;
}

void power_init(void)
{
    /* Configure GPA6 as input and wait a short while */
    GPIOA_DIR &= ~(1<<6);

    udelay(10);

    /* Value of GPA6 determines PMU chip type */
    if (GPIOA & (1<<6))
    {
        pmu = PCF50635;

        pcf50635_init();
    }
    else
    {
        pmu = PCF50606;

        /* Configure GPA6 for output (backlight enable) */
        GPIOA_DIR |= (1<<6);

        pcf50606_init();

        /* Clear pending interrupts */
        unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */
        pcf50606_read_multiple(0x02, data, 3);

#ifndef BOOTLOADER
        IEN |= EXT3_IRQ_MASK;   /* Unmask EXT3 */
#endif
    }
}

void power_off(void)
{
    /* Turn the backlight off first to avoid a bright stripe on power-off */
    _backlight_off();
    sleep(HZ/10);

    /* Power off the player using the same mechanism as the OF */
    GPIOA_CLEAR = (1<<7);
    while(true);
}

#ifndef BOOTLOADER
void EXT3(void)
{
    unsigned char data[3]; /* 0 = INT1, 1 = INT2, 2 = INT3 */

    /* Clear pending interrupts from pcf50606 */
    pcf50606_read_multiple(0x02, data, 3);

    if (data[0] & 0x04)
    {
        /* ONKEY1S */
        if (!charger_inserted())
            sys_poweroff();
        else
            pcf50606_reset_timeout();
    }

    if (data[2] & 0x08)
    {
        /* Touchscreen event, do something about it */
        touchscreen_handle_device_irq();
    }
}
#endif

#if CONFIG_CHARGING
unsigned int power_input_status(void)
{
    /* Players with a PCF50606 can use GPIOs to determine whether AC is inserted
       and whether charging is taking place. Newer players re-use C26 for the
       touchscreen, so we need to monitor PCF50635 USB/adapter IRQs for this. */
    
    if (get_pmu_type() == PCF50606)
    {
        /* Check AC adapter */
        if (GPIOD & (1<<23))
            return POWER_INPUT_MAIN_CHARGER;
        
        /* C26 indicates charging, without AC connected it implies USB power */
        if ((GPIOC & (1<<26)) == 0)
            return POWER_INPUT_USB_CHARGER;
    }
    else
    {
        /* TODO: use adapter/usb connection state from PCF50635 driver */
    }
    
    return POWER_INPUT_NONE;
}
#endif

#if CONFIG_TUNER

/** Tuner **/
static bool powered = false;

bool tuner_power(bool status)
{
    bool old_status;
    lv24020lp_lock();

    old_status = powered;

    if (status != old_status)
    {
        if (status)
        {
            /* When power up, host should initialize the 3-wire bus
               in host read mode: */

            /* 1. Set direction of the DATA-line to input-mode. */
            GPIOC_DIR &= ~(1 << 30);

            /* 2. Drive NR_W low */
            GPIOC_CLEAR = (1 << 31);
            GPIOC_DIR |= (1 << 31);

            /* 3. Drive CLOCK high */
            GPIOC_SET = (1 << 29);
            GPIOC_DIR |= (1 << 29);

            lv24020lp_power(true);
        }
        else
        {
            lv24020lp_power(false);

            /* set all as inputs */
            GPIOC_DIR &= ~((1 << 29) | (1 << 30) | (1 << 31));
        }

        powered = status;
    }

    lv24020lp_unlock();
    return old_status;
}

#endif /* CONFIG_TUNER */
