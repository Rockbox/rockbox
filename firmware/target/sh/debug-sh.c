/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Heikki Hannikainen
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
#include "system.h"
#include <stdbool.h>
#include <string.h>
#include "font.h"
#include "lcd.h"
#include "button.h"
#include "powermgmt.h"
#include "adc.h"
#include "hwcompat.h"    /* ROM_VERSION */
#include "crc32.h"      
#include "debug-target.h"


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

    volatile unsigned char* flash = (unsigned char*)0x2000000; /* flash mapping */
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
    int adc_battery_voltage;
#ifndef HAVE_LCD_BITMAP
    int currval = 0;
    int button;
#else
    int adc_battery_level;

    lcd_setfont(FONT_SYSFIXED);
#endif
    lcd_clear_display();

    while(1)
    {
#ifdef HAVE_LCD_BITMAP
        lcd_putsf(0, 0, "PADR: %04x", (unsigned short)PADR);
        lcd_putsf(0, 1, "PBDR: %04x", (unsigned short)PBDR);

        lcd_putsf(0, 2, "AN0: %03x AN4: %03x", adc_read(0), adc_read(4));
        lcd_putsf(0, 3, "AN1: %03x AN5: %03x", adc_read(1), adc_read(5));
        lcd_putsf(0, 4, "AN2: %03x AN6: %03x", adc_read(2), adc_read(6));
        lcd_putsf(0, 5, "AN3: %03x AN7: %03x", adc_read(3), adc_read(7));

        battery_read_info(&adc_battery_voltage, &adc_battery_level);
        lcd_putsf(0, 6, "Batt: %d.%03dV %d%%  ", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000, adc_battery_level);

        lcd_update();

        while (button_get_w_tmo(HZ/10) != (DEBUG_CANCEL|BUTTON_REL));

        lcd_setfont(FONT_UI);

#else /* !HAVE_LCD_BITMAP */

       if (currval == 0) {
            lcd_putsf(0, 0, "PADR: %04x", (unsigned short)PADR);
        } else if (currval == 1) {
            lcd_putsf(0, 0, "PBDR: %04x", (unsigned short)PBDR);
        } else {
            int idx = currval - 2; /* idx < 7 */
            lcd_putsf(0, 0, "AN%d: %03x", idx, adc_read(idx));
        }

        battery_read_info(&adc_battery_voltage, NULL);
        lcd_putsf(0, 1, "Batt: %d.%03dV", adc_battery_voltage / 1000,
                 adc_battery_voltage % 1000);
        lcd_update();

        button = button_get_w_tmo(HZ/5);
        switch(button)
        {
            case BUTTON_STOP:
            return false;

        case BUTTON_LEFT:
            currval--;
            if(currval < 0)
                currval = 9;
            break;

        case BUTTON_RIGHT:
            currval++;
            if(currval > 9)
                currval = 0;
            break;
        }
#endif
    }
    return false;
}

bool dbg_hw_info(void)
{
#ifndef HAVE_LCD_BITMAP
    int button;
    int currval = 0;
#else
    int bitmask = HW_MASK;
#endif
    int rom_version = ROM_VERSION;
    unsigned manu, id; /* flash IDs */
    bool got_id; /* flag if we managed to get the flash IDs */
    unsigned rom_crc = 0xffffffff; /* CRC32 of the boot ROM */
    bool has_bootrom; /* flag for boot ROM present */
    int oldmode;  /* saved memory guard mode */

    oldmode = system_memory_guard(MEMGUARD_NONE);  /* disable memory guard */

    /* get flash ROM type */
    got_id = dbg_flash_id(&manu, &id, 0x5555, 0x2AAA); /* try SST, Atmel, NexFlash */
    if (!got_id)
        got_id = dbg_flash_id(&manu, &id, 0x555, 0x2AA); /* try AMD, Macronix */

    /* check if the boot ROM area is a flash mirror */
    has_bootrom = (memcmp((char*)0, (char*)0x02000000, 64*1024) != 0);
    if (has_bootrom)  /* if ROM and Flash different */
    {
        /* calculate CRC16 checksum of boot ROM */
        rom_crc = crc_32((unsigned char*)0x0000, 64*1024, 0xffffffff);
    }

    system_memory_guard(oldmode);  /* re-enable memory guard */

    lcd_clear_display();

#ifdef HAVE_LCD_BITMAP
    lcd_setfont(FONT_SYSFIXED);

    lcd_puts(0, 0, "[Hardware info]");

    lcd_putsf(0, 1, "ROM: %d.%02d", rom_version/100, rom_version%100);

    lcd_putsf(0, 2, "Mask: 0x%04x", bitmask);
    if (got_id)
        lcd_putsf(0, 3, "Flash: M=%02x D=%02x", manu, id);
    else
        lcd_puts(0, 3, "Flash: M=?? D=??"); /* unknown, sorry */

    if (has_bootrom)
    {
        if (rom_crc == 0x56DBA4EE) /* known Version 1 */
            lcd_puts(0, 4, "Boot ROM: V1");
        else
            lcd_putsf(0, 4, "ROMcrc: 0x%08x", rom_crc);
    }
    else
    {
        lcd_puts(0, 4, "Boot ROM: none");
    }

    lcd_update();

    /* wait for exit */
    while (button_get_w_tmo(HZ/10) != (DEBUG_CANCEL|BUTTON_REL));

    lcd_setfont(FONT_UI);

#else /* !HAVE_LCD_BITMAP */
    lcd_puts(0, 0, "[HW Info]");
    while(1)
    {
        switch(currval)
        {
            case 0:
                lcd_putsf(0, 1, "ROM: %d.%02d",
                         rom_version/100, rom_version%100);
                break;
            case 1:
                if (got_id)
                    lcd_putsf(0, 1, "Flash:%02x,%02x", manu, id);
                else
                    lcd_puts(0, 1, "Flash:??,??"); /* unknown, sorry */
                break;
            case 2:
                if (has_bootrom)
                {
                    if (rom_crc == 0x56DBA4EE) /* known Version 1 */
                        lcd_puts(0, 1, "BootROM: V1");
                    else if (rom_crc == 0x358099E8)
                        lcd_puts(0, 1, "BootROM: V2");
                        /* alternative boot ROM found in one single player so far */
                    else
                        lcd_putsf(0, 1, "R: %08x", rom_crc);
                }
                else
                    lcd_puts(0, 1, "BootROM: no");
        }

        lcd_update();

        button = button_get_w_tmo(HZ/10);

        switch(button)
        {
            case BUTTON_STOP:
                return false;

            case BUTTON_LEFT:
                currval--;
                if(currval < 0)
                    currval = 2;
                break;

            case BUTTON_RIGHT:
                currval++;
                if(currval > 2)
                    currval = 0;
                break;
        }
    }
#endif
    return false;
}

