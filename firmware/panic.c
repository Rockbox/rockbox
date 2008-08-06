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

/*
 * "Dude. This is pretty fucked-up, right here." 
 */
void panicf( const char *fmt, ...)
{
    va_list ap;

#ifndef SIMULATOR
#if (CONFIG_LED == LED_REAL)
    bool state = false;
    int i = 0;
#endif

    /* Disable interrupts */
#ifdef CPU_ARM
    disable_fiq();
#endif

    set_irq_level(DISABLE_INTERRUPTS);
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
        for (i=0; i<len; i+=18) {
            unsigned char c = panic_buf[i+18];
            panic_buf[i+18] = 0;
            lcd_puts(0, y++, (unsigned char *)panic_buf+i);
            panic_buf[i+18] = c;
        }
    }
#else
    /* no LCD */
#endif
    lcd_update();
    DEBUGF(panic_buf);

    set_cpu_frequency(0);
    
#ifdef HAVE_ATA_POWER_OFF
    ide_power_enable(false);
#endif

    while (1)
    {
#ifndef SIMULATOR
#if (CONFIG_LED == LED_REAL)
        if (--i <= 0)
        {
            state = !state;
            led(state);
            i = 240000;
        }
#endif

        /* try to restart firmware if ON is pressed */
#if defined (CPU_PP)
        /* For now, just sleep the core */
        sleep_core(CURRENT_CORE);
        #define system_reboot() nop
#elif defined (TOSHIBA_GIGABEAT_F)
        if ((GPGDAT & (1 << 0)) != 0)
#elif defined(IRIVER_H100_SERIES) || defined(IRIVER_H300_SERIES)
        if ((GPIO1_READ & 0x22) == 0) /* check for ON button and !hold */
#elif defined(IAUDIO_X5) || defined(IAUDIO_M5)
        if ((GPIO_READ & 0x0c000000) == 0x08000000) /* check for ON button and !hold */
#elif defined(IAUDIO_M3)
        if ((GPIO1_READ & 0x202) == 0x200) /* check for ON button and !hold */
#elif defined(COWON_D2)
        if (GPIOA & 0x10) /* check for power button */
#elif CONFIG_CPU == SH7034
#if CONFIG_KEYPAD == PLAYER_PAD
        if (!(PADRL & 0x20))
#elif CONFIG_KEYPAD == RECORDER_PAD
#ifdef HAVE_FMADC
        if (!(PCDR & 0x0008))
#else
        if (!(PBDRH & 0x01))
#endif
#elif CONFIG_KEYPAD == ONDIO_PAD
        if (!(PCDR & 0x0008))
#endif /* CONFIG_KEYPAD */
#elif defined(CREATIVE_ZVx)
        if(false)
#elif defined(ONDA_VX747)
        /* check for power button without including any .h file */
        if( (~(*(volatile unsigned int *)(0xB0010300))) & (1 << 29) )
#endif /* CPU */
            system_reboot();
#endif /* !SIMULATOR */
    }
}
