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
    unsigned line = 0;

    lcd_setfont(FONT_SYSFIXED);
    lcd_set_viewport(NULL);
    lcd_clear_display();
    lcd_puts(0, line++, uiename[num]);
    lcd_putsf(0, line++, "at %08x" IF_COP(" (%d)"), pc
             IF_COP(, CURRENT_CORE));

#if !defined(CPU_ARM7TDMI) /* arm7tdmi has no MPU/MMU */
    if(num == 1 || num == 2) /* prefetch / data abort */
    {
        register unsigned status;

#if ARM_ARCH >= 6
        /* ARMv6 has 2 different registers for prefetch & data aborts */
        if(num == 1)    /* instruction prefetch abort */
            asm volatile( "mrc p15, 0, %0, c5, c0, 1\n" : "=r"(status));
        else
#endif
            asm volatile( "mrc p15, 0, %0, c5, c0, 0\n" : "=r"(status));

        lcd_putsf(0, line++, "FSR 0x%x", status);

        unsigned int domain = (status >> 4) & 0xf;
        unsigned int fault = status & 0xf;
#if ARM_ARCH >= 6
        fault |= (status & (1<<10)) >> 6; /* fault is 5 bits on armv6 */
#endif
        lcd_putsf(0, line++, "(domain %d, fault %d)", domain, fault);

        if(num == 2) /* data abort */
        {
            register unsigned address;
            /* read FAR (fault address register) */
            asm volatile( "mrc p15, 0, %0, c6, c0\n" : "=r"(address));
            lcd_putsf(0, line++, "address 0x%8x", address);
#if ARM_ARCH >= 6
            lcd_putsf(0, line++, (status & (1<<11)) ? "(write)" : "(read)");
#endif
        }
    }   /* num == 1 || num == 2 // prefetch/data abort */
#endif /* !defined(CPU_ARM7TDMI */

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
