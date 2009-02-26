/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "cpu.h"
#include "system.h"
#include "string.h"
#include <stdbool.h>
#include "button.h"
#include "lcd.h"
#include "sprintf.h"
#include "font.h"
#include "debug-target.h"
#include "lcd-target.h"
#include "dsp-target.h"

#ifndef CREATIVE_ZVx
#include "tsc2100.h"
#endif

bool __dbg_ports(void)
{
	dsp_init();
#ifndef CREATIVE_ZVx
	tsc2100_writereg(TSDACGAIN_PAGE, TSDACGAIN_ADDRESS, 0x2020/*x0303*/);
#endif
	dsp_wake();
    return false;
}

#ifndef CREATIVE_ZVx
extern char r_buffer[5];
extern int r_button;
#endif
bool __dbg_hw_info(void)
{
    int line = 0, oldline;
	int button;
#ifndef CREATIVE_ZVx
    int *address=0x0;
#endif
    bool done=false;
    char buf[100];

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    /* Put all the static text befor the while loop */
    lcd_puts(0, line++, "[Hardware info]");

    lcd_puts(0, line++, "Clock info:");
#if LCD_WIDTH > 320
    snprintf(buf, sizeof(buf), "IO_CLK_PLLA: 0x%04x IO_CLK_PLLB: 0x%04x IO_CLK_SEL0: 0x%04x IO_CLK_SEL1: 0x%04x",
        IO_CLK_PLLA, IO_CLK_PLLB, IO_CLK_SEL0, IO_CLK_SEL1);    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), "IO_CLK_SEL2: 0x%04x IO_CLK_DIV0: 0x%04x IO_CLK_DIV1: 0x%04x IO_CLK_DIV2: 0x%04x",
        IO_CLK_SEL2, IO_CLK_DIV0, IO_CLK_DIV1, IO_CLK_DIV2);    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), "IO_CLK_DIV3: 0x%04x IO_CLK_DIV4: 0x%04x IO_CLK_BYP : 0x%04x IO_CLK_INV : 0x%04x",
        IO_CLK_DIV3, IO_CLK_DIV4, IO_CLK_BYP, IO_CLK_INV);    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), "IO_CLK_MOD0: 0x%04x IO_CLK_MOD1: 0x%04x IO_CLK_MOD2: 0x%04x IO_CLK_LPCTL0: 0x%04x",
        IO_CLK_MOD0, IO_CLK_MOD1, IO_CLK_MOD2, IO_CLK_LPCTL0);    lcd_puts(0, line++, buf);
#else
    snprintf(buf, sizeof(buf), " IO_CLK_PLLA: 0x%04x IO_CLK_PLLB: 0x%04x", IO_CLK_PLLA, IO_CLK_PLLB);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), " IO_CLK_SEL0: 0x%04x IO_CLK_SEL1: 0x%04x", IO_CLK_SEL0, IO_CLK_SEL1);
    lcd_puts(0, line++, buf); 
    snprintf(buf, sizeof(buf), " IO_CLK_SEL2: 0x%04x IO_CLK_DIV0: 0x%04x", IO_CLK_SEL2, IO_CLK_DIV0);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), " IO_CLK_DIV1: 0x%04x IO_CLK_DIV2: 0x%04x", IO_CLK_DIV1, IO_CLK_DIV2);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), " IO_CLK_DIV3: 0x%04x IO_CLK_DIV4: 0x%04x", IO_CLK_DIV3, IO_CLK_DIV4);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), " IO_CLK_BYP : 0x%04x IO_CLK_INV : 0x%04x", IO_CLK_BYP, IO_CLK_INV);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), " IO_CLK_MOD0: 0x%04x IO_CLK_MOD1: 0x%04x ", IO_CLK_MOD0, IO_CLK_MOD1);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), " IO_CLK_MOD2: 0x%04x IO_CLK_LPCTL0: 0x%04x ", IO_CLK_MOD2, IO_CLK_LPCTL0);
    lcd_puts(0, line++, buf); 
    lcd_puts(0, line++, "Interrupt info:");
    snprintf(buf, sizeof(buf), " IO_INTC_EINT0: 0x%04x IO_INTC_EINT1: 0x%04x ", IO_INTC_EINT0, IO_INTC_EINT1);
    lcd_puts(0, line++, buf); 
    snprintf(buf, sizeof(buf), " IO_INTC_EINT2: 0x%04x IO_INTC_IRQ0: 0x%04x ", IO_INTC_EINT2, IO_INTC_IRQ0);
    lcd_puts(0, line++, buf);
    snprintf(buf, sizeof(buf), " IO_INTC_IRQ1: 0x%04x IO_INTC_IRQ2: 0x%04x ", IO_INTC_IRQ1, IO_INTC_IRQ2);
    lcd_puts(0, line++, buf);
#endif
		
    lcd_puts(0, line++, "Board revision:");
	switch (IO_BUSC_REVR) {
			case 0x0010:
					lcd_puts(0, line++, " DM320 Rev. A");
					break;
			case 0x0011:
					lcd_puts(0, line++, " DM320 Rev. B/C");
					break;
			default:
					lcd_puts(0, line++, " Unknown DM320 Chip ID");
	}

#ifndef CREATIVE_ZVx
    line++;
#endif
    oldline=line;
    while(!done)
    {
        line = oldline;
#ifndef CREATIVE_ZVx
        button = button_get(false);
        button&=~BUTTON_REPEAT;
        if (button == BUTTON_POWER)
            done=true;
        if(button==BUTTON_RC_PLAY)
            address+=0x01;
        else if (button==BUTTON_RC_DOWN)
            address-=0x01;
        else if (button==BUTTON_RC_FF)
            address+=0x800;
        else if (button==BUTTON_RC_REW)
            address-=0x800;

        snprintf(buf, sizeof(buf), "Buffer: 0x%02x%02x%02x%02x%02x",
            r_buffer[0], r_buffer[1], r_buffer[2], r_buffer[3],r_buffer[4] );    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Button: 0x%08x, HWread: 0x%08x",
            (unsigned int)button, r_button);  lcd_puts(0, line++, buf);
#else
     button = button_get(false);
     if(button & BUTTON_POWER)
        done = true;
    else if(button & BUTTON_LEFT)
        lcd_set_direct_fb(false);
    else if(button & BUTTON_RIGHT)
        lcd_set_direct_fb(true);
    
    lcd_puts(0, line++, "LCD info:");
    snprintf(buf, sizeof(buf), " LCD direct FB access? %s", (lcd_get_direct_fb() ? "yes" : "no"));
    lcd_puts(0, line++, buf);
    line++;
#endif
        lcd_puts(0, line++, "[Rockbox info]");
        snprintf(buf, sizeof(buf), "current tick: %08x Seconds running: %08d",
            (unsigned int)current_tick, (unsigned int)current_tick/100);  lcd_puts(0, line++, buf);
#ifndef CREATIVE_ZVx
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x", 
            (unsigned int)address, *address);           lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x",
            (unsigned int)(address+1), *(address+1));   lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "Address: 0x%08x Data: 0x%08x",
            (unsigned int)(address+2), *(address+2));   lcd_puts(0, line++, buf);
#endif

        lcd_update();
    }
    return false;
}
