/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "config.h"
#include "jz4740.h"
#include "backlight.h"
#include "font.h"
#include "lcd.h"
#include "system.h"
#include "mips.h"
#include "button.h"

int _line = 1;
char _printfbuf[256];

/* This is all rather hacky, but it works... */
void _printf(const char *format, ...)
{
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    ptr = _printfbuf;
    len = vsnprintf(ptr, sizeof(_printfbuf), format, ap);
    va_end(ap);

    int i;
    for(i=0; i<1; i++)
    {
        lcd_puts(0, _line++, ptr);
        lcd_update();
    }
    if(_line >= LCD_HEIGHT/SYSFONT_HEIGHT)
        _line = 1;
}

void audiotest(void)
{
    __i2s_internal_codec();
    __aic_enable();
    __aic_reset();
    __aic_select_i2s();
    __aic_enable_loopback();
}

static void jz_store_icache(void)
{
    unsigned long start;
    unsigned long end;

    start = KSEG0BASE;
    end = start + CFG_ICACHE_SIZE;
    while(start < end)
    {
        cache_unroll(start, 8);
        start += CFG_CACHELINE_SIZE;
    }
}

int main(void)
{
    cli();
    
    write_c0_status(0x10000400);
    
    memcpy((void *)A_K0BASE, (void *)0x80E00080, 0x20);
    memcpy((void *)(A_K0BASE + 0x180), (void *)0x80E00080, 0x20);
    memcpy((void *)(A_K0BASE + 0x200), (void *)0x80E00080, 0x20);
    
    jz_flush_dcache();
    jz_store_icache();
    
    sti();
    
    kernel_init();
    lcd_init();
    font_init();
    lcd_setfont(FONT_SYSFIXED);
    button_init();
    
    backlight_init();
    
    /* To make the Windows say "ding-dong".. */
    REG8(USB_REG_POWER) &= ~USB_POWER_SOFTCONN;

    int touch;
    lcd_clear_display();
    _printf("Rockbox bootloader v0.000001");
    while(1)
    {
        if(button_read_device(&touch) & BUTTON_VOL_DOWN)
            _printf("BUTTON_VOL_DOWN");
        if(button_read_device(&touch) & BUTTON_MENU)
            _printf("BUTTON_MENU");
        if(button_read_device(&touch) & BUTTON_VOL_UP)
            _printf("BUTTON_VOL_UP");
        if(button_read_device(&touch) & BUTTON_POWER)
            _printf("BUTTON_POWER");
        _printf("X: %d Y: %d", touch>>16, touch&0xFFFF);
    }
    
    return 0;
}
