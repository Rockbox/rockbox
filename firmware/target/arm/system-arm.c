/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Thom Johansen
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
#include <stdio.h>
#include "lcd.h"
#include "font.h"

static const char* const uiename[] = {
    "Undefined instruction",
    "Prefetch abort",
    "Data abort",
    "Divide by zero"
};

/* Unexpected Interrupt or Exception handler. Currently only deals with
   exceptions, but will deal with interrupts later.
 */
void __attribute__((noreturn)) UIE(unsigned int pc, unsigned int num)
{
#if LCD_DEPTH > 1
    lcd_set_backdrop(NULL);
    lcd_set_drawmode(DRMODE_SOLID);
    lcd_set_foreground(LCD_BLACK);
    lcd_set_background(LCD_WHITE);
#endif
    lcd_setfont(FONT_SYSFIXED);
    lcd_set_viewport(NULL);
#endif
    lcd_clear_display();
    lcd_puts(0, 0, uiename[num]);
    lcd_putsf(0, 1, "at %08x" IF_COP(" (%d)"), pc
             IF_COP(, CURRENT_CORE));
    lcd_update();

    disable_interrupt(IRQ_FIQ_STATUS);

    system_exception_wait(); /* If this returns, try to reboot */
    system_reboot();
    while (1);       /* halt */
}

/* Needs to be here or gcc won't find it */
void __attribute__((naked)) __div0(void)
{
    asm volatile (
        "ldr    r0, [sp] \r\n"
        "mov    r1, #3   \r\n"
        "b      UIE      \r\n"
    );
}
