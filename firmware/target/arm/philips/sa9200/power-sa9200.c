/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Daniel Ankers
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

#include <stdbool.h>
#include "system.h"
#include "cpu.h"
#include "ascodec.h"
#include "as3514.h"
#include "power.h"
#include "synaptics-mep.h"
#include "lcd.h"
#include "logf.h"

void power_init(void)
{
#ifndef BOOTLOADER
    /* Power on and initialize the touchpad here because we need it for
       both buttons and button lights */
    DEV_INIT2 &= ~0x800;

    char byte = ascodec_read(AS3514_CVDD_DCDC3);
    byte = (byte & ~0x18) | 0x08;
    ascodec_write(AS3514_CVDD_DCDC3, byte);

    /* LEDs for REW, FFWD, MENU */
    GPIOD_ENABLE     |=  (0x40 | 0x20 | 0x04);
    GPIOD_OUTPUT_VAL |=  (0x40 | 0x20 | 0x04);
    GPIOD_OUTPUT_EN  |=  (0x40 | 0x20 | 0x04);
    udelay(20000);

    GPIOL_ENABLE     |=  0x10;
    GPIOL_OUTPUT_VAL &= ~0x10;
    GPIOL_OUTPUT_EN  |=  0x10;
    udelay(100000);

    /* enable DATA, ACK, CLK lines */
    GPIOD_ENABLE     |=  (0x10 | 0x08 | 0x02);

    GPIOD_OUTPUT_EN  |=  0x08; /* ACK  */
    GPIOD_OUTPUT_VAL |=  0x08; /* high */

    GPIOD_OUTPUT_EN  &= ~0x02; /* CLK  */
    
    GPIOD_OUTPUT_EN  |=  0x10; /* DATA */
    GPIOD_OUTPUT_VAL |=  0x10; /* high */

    if (!touchpad_init())
    {
        logf("touchpad not ready");
    }

    /* Setups specifically for buttons are handled in button-sa9200.c */
#endif
}

void power_off(void)
{
    char byte;

    /* Backlight off */
    ascodec_write(AS3514_DCDC15, 0);

#ifdef HAVE_LCD_SLEEP
    /* LCD off/sleep (otherwise the image slowly fades out) */
    lcd_sleep();
#endif

    /* Send shutdown command to PMU */
    byte = ascodec_read(AS3514_SYSTEM);
    byte &= ~0x1;   
    ascodec_write(AS3514_SYSTEM, byte);

    /* Stop interrupts on both cores */
    disable_interrupt(IRQ_FIQ_STATUS);
    COP_INT_DIS = -1;
    CPU_INT_DIS = -1;

    /* Halt everything and wait for device to power off */
    while (1)
    {
        COP_CTL = 0x40000000;
        CPU_CTL = 0x40000000;
    }
}

unsigned int power_input_status(void)
{
    unsigned int status = POWER_INPUT_NONE;

    /* GPIOF indicates that the connector is present,
       GPIOB indicates that there's power there too.
       Same status bits for both USB and the charger. */
    if (!(GPIOF_INPUT_VAL & 0x80) && (GPIOB_INPUT_VAL & 0x40))
        status = POWER_INPUT_MAIN_CHARGER;

    return status;
}

void ide_power_enable(bool on)
{
    (void)on;
}
