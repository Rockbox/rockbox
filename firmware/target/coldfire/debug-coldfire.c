/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
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

#include "config.h"
#include "system.h"
#include "kernel.h"
#include "font.h"
#include "lcd.h"
#include "button.h"
#include "powermgmt.h"
#include "adc.h"
#include "lcd-remote.h"
#ifdef IAUDIO_X5
#include "ds2411.h"
#endif

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
   (CONFIG_KEYPAD == IRIVER_H300_PAD)
#   define DEBUG_CANCEL  BUTTON_OFF

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#   define DEBUG_CANCEL  BUTTON_REC

#elif (CONFIG_KEYPAD == IAUDIO_M3_PAD)
#   define DEBUG_CANCEL  BUTTON_RC_REC

#elif (CONFIG_KEYPAD == MPIO_HD200_PAD)
#   define DEBUG_CANCEL  BUTTON_REC

#elif (CONFIG_KEYPAD == MPIO_HD300_PAD)
#   define DEBUG_CANCEL BUTTON_MENU
#endif

/* dbg_flash_id() hits address 0, which is nominally illegal.  Make sure
   GCC doesn't helpfully turn this into an exception.
 */
#pragma GCC optimize "no-delete-null-pointer-checks"
/* Tool function to read the flash manufacturer and type, if available.
   Only chips which could be reprogrammed in system will return values.
   (The mode switch addresses vary between flash manufacturers, hence addr1/2) */
   /* In IRAM to avoid problems when running directly from Flash */
static bool dbg_flash_id(unsigned* p_manufacturer, unsigned* p_device,
                         unsigned addr1, unsigned addr2)
                         ICODE_ATTR __attribute__((noinline));
static bool dbg_flash_id(unsigned* p_manufacturer, unsigned* p_device,
                         unsigned addr1, unsigned addr2)

{
    unsigned not_manu, not_id; /* read values before switching to ID mode */
    unsigned manu, id; /* read values when in ID mode */

    volatile unsigned short* flash = (unsigned short*)0; /* flash mapping */
    int old_level; /* saved interrupt level */

    not_manu = flash[0]; /* read the normal content */
    not_id   = flash[1]; /* should be 'A' (0x41) and 'R' (0x52) from the "ARCH" marker */

    /* disable interrupts, prevent any stray flash access */
    old_level = disable_irq_save();

    flash[addr1] = 0xAA; /* enter command mode */
    flash[addr2] = 0x55;
    flash[addr1] = 0x90; /* ID command */
    /* Atmel wants 20ms pause here */
    /* sleep(HZ/50); no sleeping possible while interrupts are disabled */
    manu = flash[0]; /* read the IDs */
    id   = flash[1];

    flash[0] = 0xF0; /* reset flash (back to normal read mode) */
    /* Atmel wants 20ms pause here */
    /* sleep(HZ/50); no sleeping possible while interrupts are disabled */

    restore_irq(old_level); /* enable interrupts again */

    /* I assume success if the obtained values are different from
        the normal flash content. This is not perfectly bulletproof, they
        could theoretically be the same by chance, causing us to fail. */
    if (not_manu != manu || not_id != id) /* a value has changed */
    {
        *p_manufacturer = manu; /* return the results */
        *p_device = id;
        return true; /* success */
    }
    return false; /* fail */
}

bool dbg_ports(void)
{
    unsigned int gpio_out;
    unsigned int gpio1_out;
    unsigned int gpio_read;
    unsigned int gpio1_read;
    unsigned int gpio_function;
    unsigned int gpio1_function;
    unsigned int gpio_enable;
    unsigned int gpio1_enable;
    int adc_battery_voltage, adc_battery_level;
    int adc_buttons, adc_remote;
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        gpio_read = GPIO_READ;
        gpio1_read = GPIO1_READ;
        gpio_out = GPIO_OUT;
        gpio1_out = GPIO1_OUT;
        gpio_function = GPIO_FUNCTION;
        gpio1_function = GPIO1_FUNCTION;
        gpio_enable = GPIO_ENABLE;
        gpio1_enable = GPIO1_ENABLE;

        lcd_putsf(0, line++, "GPIO_READ: %08x", gpio_read);
        lcd_putsf(0, line++, "GPIO_OUT:  %08x", gpio_out);
        lcd_putsf(0, line++, "GPIO_FUNC: %08x", gpio_function);
        lcd_putsf(0, line++, "GPIO_ENA:  %08x", gpio_enable);
        lcd_putsf(0, line++, "GPIO1_READ: %08x", gpio1_read);
        lcd_putsf(0, line++, "GPIO1_OUT:  %08x", gpio1_out);
        lcd_putsf(0, line++, "GPIO1_FUNC: %08x", gpio1_function);
        lcd_putsf(0, line++, "GPIO1_ENA:  %08x", gpio1_enable);

        adc_buttons = adc_read(ADC_BUTTONS);
        adc_remote  = adc_read(ADC_REMOTE);
        battery_read_info(&adc_battery_voltage, &adc_battery_level);
#if defined(IAUDIO_X5) ||  defined(IAUDIO_M5) || defined(IRIVER_H300_SERIES)
        lcd_putsf(0, line++, "ADC_BUTTONS (%c): %02x",
            button_scan_enabled() ? '+' : '-', adc_buttons);
#else
        lcd_putsf(0, line++, "ADC_BUTTONS: %02x", adc_buttons);
#endif
#if defined(IAUDIO_X5) || defined(IAUDIO_M5)
        lcd_putsf(0, line++, "ADC_REMOTE  (%c): %02x",
            remote_detect() ? '+' : '-', adc_remote);
#else
        lcd_putsf(0, line++, "ADC_REMOTE:  %02x", adc_remote);
#endif
#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        lcd_putsf(0, line++, "ADC_REMOTEDETECT: %02x",
                 adc_read(ADC_REMOTEDETECT));
#endif

        battery_read_info(&adc_battery_voltage, &adc_battery_level);
        lcd_putsf(0, line++, "Batt: %d.%03dV %d%%  ", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000, adc_battery_level);

#if defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        lcd_putsf(0, line++, "remotetype: %d", remote_type());
#endif

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            lcd_setfont(FONT_UI);
            return false;
        }
    }
    return false;
}

bool dbg_hw_info(void)
{
    unsigned manu, id; /* flash IDs */
    int got_id; /* flag if we managed to get the flash IDs */
    int oldmode;  /* saved memory guard mode */
    int line = 0;

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */

    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    lcd_puts(0, line++, "[Hardware info]");

    if (got_id)
        lcd_putsf(0, line++, "Flash: M=%04x D=%04x", manu, id);
    else
        lcd_puts(0, line++, "Flash: M=???? D=????"); /* unknown, sorry */

#ifdef IAUDIO_X5
    {
        struct ds2411_id id;

        lcd_puts(0, ++line, "Serial Number:");

        got_id = ds2411_read_id(&id);

        if (got_id == DS2411_OK)
        {
            lcd_putsf(0, ++line, "  FC=%02x", (unsigned)id.family_code);
            lcd_putsf(0, ++line, "  ID=%02X %02X %02X %02X %02X %02X",
                (unsigned)id.uid[0], (unsigned)id.uid[1], (unsigned)id.uid[2],
                (unsigned)id.uid[3], (unsigned)id.uid[4], (unsigned)id.uid[5]);
            lcd_putsf(0, ++line, "  CRC=%02X", (unsigned)id.crc);
        }
        else
        {
            lcd_putsf(0, ++line, "READ ERR=%d", got_id);
        }
    }
#endif

    lcd_update();

    /* wait for exit */
    while (button_get_w_tmo(HZ/10) != (DEBUG_CANCEL|BUTTON_REL));

    lcd_setfont(FONT_UI);
    return false;
}
