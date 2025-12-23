/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2025 by Aidan MacDonald
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
#include "kernel/kernel-internal.h"
#include "system.h"
#include "power.h"
#include "rtc.h"
#include "lcd.h"
#include "backlight.h"
#include "button.h"
#include "timefuncs.h"
#include "storage.h"
#include "disk.h"
#include "file_internal.h"

extern void show_logo(void);

static void demo_rtc(void)
{
    int y = 0;
    struct tm *time = get_time();

    lcd_clear_display();

    lcd_putsf(0, y++, "time:  %02d:%02d:%02d",
              time->tm_hour, time->tm_min, time->tm_sec);
    lcd_putsf(0, y++, "year:  %d", time->tm_year + 1900);
    lcd_putsf(0, y++, "month: %d", time->tm_mon);
    lcd_putsf(0, y++, "day:   %d", time->tm_mday);

    lcd_update();
}

static void demo_storage(void)
{
    int y = 0;

    lcd_clear_display();
    lcd_putsf(0, y++, "tick %ld", current_tick);

    struct partinfo pinfo;
    if (storage_present(IF_MD(0,)) && disk_partinfo(0, &pinfo))
    {
        lcd_putsf(0, y++, "start %d", (int)pinfo.start);
        lcd_putsf(0, y++, "count %d", (int)pinfo.size);
        lcd_putsf(0, y++, "type  %d", (int)pinfo.type);

        DIR *d = opendir("/");
        struct dirent *ent;
        while ((ent = readdir(d)))
        {
            lcd_putsf(0, y++, "/%s", ent->d_name);
        }

        closedir(d);
    }

    lcd_update();
}

static void (*demo_funcs[]) (void) = {
    demo_rtc,
    demo_storage,
};

void main(void)
{
    system_init();
    kernel_init();
    rtc_init();

    lcd_init();
    button_init();

    backlight_init();
    backlight_on();

    show_logo();

    storage_init();
    filesystem_init();
    disk_mount_all();

    int demo_page = 0;
    const int num_pages = ARRAYLEN(demo_funcs);

    while (1)
    {
        int btn = button_get_w_tmo(HZ);
        switch (btn)
        {
        case BUTTON_START:
            demo_page += 1;
            if (demo_page >= num_pages)
                demo_page = 0;
            break;

        case BUTTON_SELECT:
            if (demo_page == 0)
                demo_page = num_pages - 1;
            else
                demo_page -= 1;
            break;

        default:
            break;
        }

        demo_funcs[demo_page]();
    }
}
