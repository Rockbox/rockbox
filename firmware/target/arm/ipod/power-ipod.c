/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include <stdbool.h>
#include "adc.h"
#include "kernel.h"
#include "system.h"
#include "power.h"
#include "logf.h"
#include "pcf50605.h"
#include "usb.h"
#include "lcd.h"

void power_init(void)
{
#if defined(IPOD_1G2G) || defined(IPOD_3G)
    GPIOC_ENABLE |= 0x40;      /* GPIO C6 is HDD power (low active) */
    GPIOC_OUTPUT_VAL &= ~0x40; /* on by default */
    GPIOC_OUTPUT_EN |= 0x40;   /* enable output */
#endif
#ifndef IPOD_1G2G
    pcf50605_init();
#endif
}

bool charger_inserted(void)
{
#if defined(IPOD_VIDEO)
    return (GPIOL_INPUT_VAL & 0x08)?false:true;
#elif defined(IPOD_4G) || defined(IPOD_COLOR) \
       || defined(IPOD_MINI) || defined(IPOD_MINI2G)
    /* C2 is firewire power */
    return (GPIOC_INPUT_VAL & 0x04)?false:true; 
#else
    /* This needs filling in for other ipods. */
    return false;
#endif
}

/* Returns true if the unit is charging the batteries. */
bool charging_state(void) {
    return (GPIOB_INPUT_VAL & 0x01)?false:true;
}


void ide_power_enable(bool on)
{
#if defined(IPOD_1G2G) || defined(IPOD_3G)
    if (on)
        GPIOC_OUTPUT_VAL &= ~0x40;
    else
        GPIOC_OUTPUT_VAL |= 0x40;
#elif defined(IPOD_4G) || defined(IPOD_COLOR) \
   || defined(IPOD_MINI) || defined(IPOD_MINI2G)
    if (on)
    {
        GPIO_CLEAR_BITWISE(GPIOJ_OUTPUT_VAL, 0x04);
        DEV_EN |= DEV_IDE0;
    }
    else
    {
        DEV_EN &= ~DEV_IDE0;
        GPIO_SET_BITWISE(GPIOJ_OUTPUT_VAL, 0x04);
    }
#elif defined(IPOD_VIDEO)
    if (on)
    {
        GPO32_VAL &= ~0x40000000;
        DEV_EN |= DEV_IDE0;
    }
    else
    {
        DEV_EN &= ~DEV_IDE0;
        GPO32_VAL |= 0x40000000;
    }
#else /* Nano */
    (void)on;  /* Do nothing. */
#endif
}

bool ide_powered(void)
{
#if defined(IPOD_1G2G) || defined(IPOD_3G)
    return !(GPIOC_OUTPUT_VAL & 0x40);
#elif defined(IPOD_4G) || defined(IPOD_COLOR) \
   || defined(IPOD_MINI) || defined(IPOD_MINI2G)
    return !(GPIOJ_OUTPUT_VAL & 0x04);
#elif defined(IPOD_VIDEO)
    return !(GPO32_VAL & 0x40000000);
#else /* Nano */
    return true; /* Pretend we are always powered */
#endif
}

void power_off(void)
{
#if defined(HAVE_LCD_COLOR)
    /* Clear the screen and backdrop to
    remove ghosting effect on shutdown */
    lcd_set_backdrop(NULL);
    lcd_set_background(LCD_WHITE);
    lcd_clear_display();
    lcd_update();
    sleep(HZ/16);
#endif

#ifndef BOOTLOADER
#ifdef IPOD_1G2G
    /* we cannot turn off the 1st gen/ 2nd gen yet. Need to figure out sleep mode. */
    system_reboot();
#else
    /* We don't turn off the ipod, we put it in a deep sleep */
    pcf50605_standby_mode();
#endif
#endif
}
