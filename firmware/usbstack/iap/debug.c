/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2025 by Sho Tanimoto
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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "logf.h"
#include "system.h"
#include "tick.h"

#include "font.h"
#include "lcd.h"

#define MAX_COLS 64

static int      rows;
static int      columns;
static unsigned count;

static void update_color(void) {
    unsigned avail = rows - 1;
    if((count % (avail * 2)) > avail) {
        lcd_set_drawinfo(DRMODE_SOLID, LCD_BLACK, LCD_WHITE);
    } else {
        lcd_set_drawinfo(DRMODE_SOLID, LCD_WHITE, LCD_BLACK);
    }
}

void iap_lcd_scatter(const char* fmt, ...) {
    if(rows == 0) {
        int w, h;
        font_getstringsize((unsigned char*)"A", &w, &h, FONT_SYSFIXED);
        columns = MIN(LCD_WIDTH / w, MAX_COLS);
        rows    = LCD_HEIGHT / h;
    }

    va_list ap;
    char    buf[256];
    va_start(ap, fmt);
    int len = vsnprintf(buf, sizeof(buf), (char*)fmt, ap);
    va_end(ap);

    logf("%s", buf);

    lcd_set_backdrop(NULL);
    lcd_setfont(FONT_SYSFIXED);

    update_color();
    lcd_putsf(0, 0, (unsigned char*)"count %u", count);
    for(int i = 0; i < len;) {
        char line[MAX_COLS];
        int  copy = MIN(len - i, columns - 1);
        memcpy(line, buf + i, copy);
        memset(line + copy, ' ', columns - 1 - copy);
        line[columns - 1] = '\0';
        update_color();
        lcd_puts(0, 1 + count % (rows - 1), (unsigned char*)line);
        count += 1;
        i += copy;
    }
    lcd_update();
}

static unsigned long timestamp_epoch;

unsigned long iap_debug_timestamp(void) {
    return current_tick - timestamp_epoch;
}

void iap_debug_reset_timestamp(void) {
    timestamp_epoch = current_tick;
}
