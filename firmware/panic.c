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
 *nn
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

/*
 * "Dude. This is pretty fucked-up, right here." 
 */
void panicf( const char *fmt, ...)
{
    va_list ap;

#ifndef SIMULATOR
#if CONFIG_LED == LED_REAL
    bool state = false;
#endif

    /* Disable interrupts */
#if CONFIG_CPU == SH7034
    asm volatile ("ldc\t%0,sr" : : "r"(15<<4));
#elif defined(CPU_COLDFIRE)
    asm volatile ("move.w #0x2700,%sr");
#endif
#endif

    va_start( ap, fmt );
    vsnprintf( panic_buf, sizeof(panic_buf), fmt, ap );
    va_end( ap );

#ifdef HAVE_LCD_CHARCELLS
    lcd_double_height(false);
    lcd_puts(0,0,"*PANIC*");
    lcd_puts(0,1,panic_buf);
#elif defined(HAVE_LCD_BITMAP)
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);
    lcd_puts(0,0,"*PANIC*");
    {
        /* wrap panic line */
        int i, y=1, len = strlen(panic_buf);
        for (i=0; i<len; i+=18) {
            unsigned char c = panic_buf[i+18];
            panic_buf[i+18] = 0;
            lcd_puts(0,y++,panic_buf+i);
            panic_buf[i+18] = c;
        }
    }
    lcd_update();

#else
    /* no LCD */
#endif
    DEBUGF(panic_buf);

    set_cpu_frequency(0);
    
#ifdef HAVE_ATA_POWER_OFF
    ide_power_enable(false);
#endif

    while (1)
    {
#if (CONFIG_LED == LED_REAL) && !defined(SIMULATOR)
        volatile long i;
        led (state);
        state = state?false:true;
        
        for (i = 0; i < 240000; ++i);
#endif
#ifndef SIMULATOR
#ifdef IRIVER_H100_SERIES
        /* check for the ON button (and !hold) */
        if ((GPIO1_READ & 0x22) == 0)
            system_reboot();
#elif CONFIG_CPU == SH7034
        /* try to restart firmware if ON is pressed */
#if CONFIG_KEYPAD == PLAYER_PAD
        if (!(PADR & 0x0020))
#elif CONFIG_KEYPAD == RECORDER_PAD
#ifdef HAVE_FMADC
        if (!(PCDR & 0x0008))
#else
        if (!(PBDR & 0x0100))
#endif
#endif
            system_reboot();
#endif
#endif
    }
}
