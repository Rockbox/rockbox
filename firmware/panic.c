/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by wavey@wavey.org
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
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "panic.h"
#include "lcd.h"
#include "font.h"
#include "debug.h"
#include "led.h"
#include "power.h"
#include "system.h"

static char panic_buf[128];
#define LINECHARS (LCD_WIDTH/SYSFONT_WIDTH)

/*
 * "Dude. This is pretty fucked-up, right here." 
 */
void panicf( const char *fmt, ...)
{
    va_list ap;

#ifndef SIMULATOR
    /* Disable interrupts */
#ifdef CPU_ARM
    disable_interrupt(IRQ_FIQ_STATUS);
#else
    set_irq_level(DISABLE_INTERRUPTS);
#endif
#endif /* SIMULATOR */

    va_start( ap, fmt );
    vsnprintf( panic_buf, sizeof(panic_buf), fmt, ap );
    va_end( ap );

#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height(false);
    lcd_puts(0, 0, "*PANIC*");
    lcd_puts(0, 1, panic_buf);
#elif defined(HAVE_LCD_BITMAP)
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);
    lcd_puts(0, 0, (unsigned char *)"*PANIC*");
    {
        /* wrap panic line */
        int i, y=1, len = strlen(panic_buf);
        for (i=0; i<len; i+=LINECHARS) {
            unsigned char c = panic_buf[i+LINECHARS];
            panic_buf[i+LINECHARS] = 0;
            lcd_puts(0, y++, (unsigned char *)panic_buf+i);
            panic_buf[i+LINECHARS] = c;
        }
    }
#else
    /* no LCD */
#endif
    lcd_update();
    DEBUGF("%s", panic_buf);

    set_cpu_frequency(0);
    
#ifdef HAVE_ATA_POWER_OFF
    ide_power_enable(false);
#endif

    system_exception_wait(); /* if this returns, try to reboot */
    system_reboot();
    while (1);       /* halt */
}
