/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2020 by Solomon Peachy
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "config.h"
#include "font.h"
#include "lcd.h"
#include "kernel.h"
#include "button.h"

#ifndef BOOTLOADER

#include "pcm-alsa.h"

static int line = 0;

extern int hwver;

bool dbg_hw_info(void)
{
    int btn = 0;

    /* Try to read the bootloader */
    char verstr[40];
    memset(verstr, 0, sizeof(verstr));
    int fd = open("/etc/rockbox-bl-info.txt", O_RDONLY);
    if(fd >= 0)
    {
        read(fd, verstr, sizeof(verstr) -1);
        close(fd);
    }

    lcd_setfont(FONT_SYSFIXED);

    while(btn ^ BUTTON_POWER) {
        lcd_clear_display();
        line = 0;

        if (verstr[0]) {
            lcd_putsf(0, line++, "Boot ver: %s", verstr);
        }

#ifdef EROS_Q
        lcd_putsf(0, line++, "hwver: %d", hwver);
#endif

        lcd_putsf(0, line++, "pcm srate: %d", pcm_alsa_get_rate());
        lcd_putsf(0, line++, "pcm xruns: %d", pcm_alsa_get_xruns());
#ifdef HAVE_HEADPHONE_DETECTION
        lcd_putsf(0, line++, "hp: %d", headphones_inserted());
#endif
#ifdef HAVE_LINEOUT_DETECTION
        lcd_putsf(0, line++, "lo: %d", lineout_inserted());
#endif

        btn = button_read_device();
        lcd_putsf(0, line++, "btn: %d", btn);

        lcd_update();
        sleep(HZ/16);
    }
    return true;
}

#endif /* !BOOTLOADER */
