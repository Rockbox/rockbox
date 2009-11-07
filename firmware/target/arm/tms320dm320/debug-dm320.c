/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007, 2009 by Karl Kurbjun
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
#include "debug.h"
#include "sprintf.h"
#include "font.h"
#include "lcd-target.h"

#if defined(MROBE_500)
#include "tsc2100.h"
#include "m66591.h"
#endif

bool __dbg_ports(void)
{
#if defined(MROBE_500)
    int line = 0;
    int i;
	int button;
    bool done=false;
    
    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();
    
    while(!done)
    {
        line = 0;
        button = button_get(false);
        button&=~BUTTON_REPEAT;
        if (button == BUTTON_POWER)
            done=true;

        lcd_puts(0, line++, "[USB Information]");
        
        lcd_putsf(0, line++, "TRN_CTRL:     0x%04x TRN_LNSTAT:   0x%04x",
            M66591_TRN_CTRL, M66591_TRN_LNSTAT);
        lcd_putsf(0, line++, "HSFS:         0x%04x TESTMODE:     0x%04x",
            M66591_HSFS, M66591_TESTMODE);
        lcd_putsf(0, line++, "PIN_CFG0:     0x%04x PIN_CFG1:     0x%04x",
            M66591_PIN_CFG0, M66591_PIN_CFG1);
        lcd_putsf(0, line++, "PIN_CFG2:     0x%04x", M66591_PIN_CFG2);
        lcd_putsf(0, line++, "DCP_CTRLEN:   0x%04x", M66591_DCP_CTRLEN);
        lcd_putsf(0, line++, "CPORT_CTRL0:  0x%04x CPORT_CTRL1:  0x%04x",
            M66591_CPORT_CTRL0, M66591_CPORT_CTRL1);
        lcd_putsf(0, line++, "CPORT_CTRL2:  0x%04x DPORT_CTRL0:  0x%04x",
            M66591_CPORT_CTRL2, M66591_DPORT_CTRL0);
        lcd_putsf(0, line++, "DPORT_CTRL1:  0x%04x DPORT_CTRL2:  0x%04x",
            M66591_DPORT_CTRL1, M66591_DPORT_CTRL2);
        lcd_putsf(0, line++, "INTCFG_MAIN:  0x%04x INTCFG_OUT:   0x%04x",
            M66591_INTCFG_MAIN, M66591_INTCFG_OUT);
        lcd_putsf(0, line++, "INTCFG_RDY:   0x%04x INTCFG_NRDY:  0x%04x",
            M66591_INTCFG_RDY, M66591_INTCFG_NRDY);
        lcd_putsf(0, line++, "INTCFG_EMP:   0x%04x INTSTAT_MAIN: 0x%04x",
            M66591_INTCFG_EMP, M66591_INTSTAT_MAIN);
        lcd_putsf(0, line++, "INTSTAT_RDY:  0x%04x INTSTAT_NRDY: 0x%04x",
            M66591_INTSTAT_RDY, M66591_INTSTAT_NRDY);
        lcd_putsf(0, line++, "INTSTAT_EMP:  0x%04x USB_ADDRESS:  0x%04x",
            M66591_INTSTAT_EMP, M66591_USB_ADDRESS);
        lcd_putsf(0, line++, "USB_REQ0:     0x%04x USB_REQ1:     0x%04x",
            M66591_USB_REQ0, M66591_USB_REQ1);  
        lcd_putsf(0, line++, "USB_REQ2:     0x%04x USB_REQ3:     0x%04x",
            M66591_USB_REQ2, M66591_USB_REQ3);  
        lcd_putsf(0, line++, "DCP_CNTMD:    0x%04x DCP_MXPKSZ:   0x%04x",
            M66591_DCP_CNTMD, M66591_DCP_MXPKSZ);  
        lcd_putsf(0, line++, "DCPCTRL:      0x%04x", M66591_DCPCTRL);  
            
        line++;
        for(i=1; i<6; i++) {
            M66591_PIPE_CFGWND=i;
            lcd_putsf(0, line++, "PIPE_CFGSEL:0x%04x PIPE_CFGWND: 0x%04x", 
                M66591_PIPE_CFGSEL, M66591_PIPE_CFGWND);
        }
        line++;
            
        lcd_putsf(0, line++, "PIPECTRL1:    0x%04x PIPECTRL2:    0x%04x",
            M66591_PIPECTRL1, M66591_PIPECTRL2);  
        lcd_putsf(0, line++, "PIPECTRL3:    0x%04x PIPECTRL4:    0x%04x",
            M66591_PIPECTRL3, M66591_PIPECTRL4);  
        lcd_putsf(0, line++, "PIPECTRL5:    0x%04x PIPECTRL6:    0x%04x",
            M66591_PIPECTRL5, M66591_PIPECTRL6);  
            
        lcd_putsf(0, line++, "GIO_BITSET0:  0x%04x GIO_BITSET1:  0x%04x",
            IO_GIO_BITSET0, IO_GIO_BITSET1);  
            
        lcd_putsf(0, line++, "GIO_BITSET2:  0x%04x", IO_GIO_BITSET2);  
            
        lcd_putsf(0, line++, "SDRAM_SDMODE: 0x%04x SDRAM_REFCTL: 0x%04x",
            IO_SDRAM_SDMODE, IO_SDRAM_REFCTL);  
            
        lcd_putsf(0, line++, "EMIF_CS4CTRL1:0x%04x EMIF_CS4CTRL2:0x%04x",
            IO_EMIF_CS4CTRL1, IO_EMIF_CS4CTRL2);  

        lcd_update();
    }
#endif
    return false;
}

bool __dbg_hw_info(void)
{
    int line = 0, oldline;
	int button;
#if defined(MROBE_500)
    int *address=0x0;
#endif
    bool done=false;

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    /* Put all the static text befor the while loop */
    lcd_puts(0, line++, "[Hardware info]");

    lcd_puts(0, line++, "Clock info:");
#if LCD_WIDTH > 320
    lcd_putsf(0, line++, "IO_CLK_PLLA: 0x%04x IO_CLK_PLLB: 0x%04x "
        "IO_CLK_SEL0: 0x%04x IO_CLK_SEL1: 0x%04x",
        IO_CLK_PLLA, IO_CLK_PLLB, IO_CLK_SEL0, IO_CLK_SEL1);
    lcd_putsf(0, line++, "IO_CLK_SEL2: 0x%04x IO_CLK_DIV0: 0x%04x "
        "IO_CLK_DIV1: 0x%04x IO_CLK_DIV2: 0x%04x",
        IO_CLK_SEL2, IO_CLK_DIV0, IO_CLK_DIV1, IO_CLK_DIV2);
    lcd_putsf(0, line++, "IO_CLK_DIV3: 0x%04x IO_CLK_DIV4: 0x%04x "
        "IO_CLK_BYP : 0x%04x IO_CLK_INV : 0x%04x",
        IO_CLK_DIV3, IO_CLK_DIV4, IO_CLK_BYP, IO_CLK_INV);
    lcd_putsf(0, line++, "IO_CLK_MOD0: 0x%04x IO_CLK_MOD1: 0x%04x "
        "IO_CLK_MOD2: 0x%04x IO_CLK_LPCTL0: 0x%04x",
        IO_CLK_MOD0, IO_CLK_MOD1, IO_CLK_MOD2, IO_CLK_LPCTL0);
#else
    lcd_putsf(0, line++, " IO_CLK_PLLA: 0x%04x IO_CLK_PLLB: 0x%04x", 
        IO_CLK_PLLA, IO_CLK_PLLB);
    lcd_putsf(0, line++, " IO_CLK_SEL0: 0x%04x IO_CLK_SEL1: 0x%04x", 
        IO_CLK_SEL0, IO_CLK_SEL1);
    lcd_putsf(0, line++, " IO_CLK_SEL2: 0x%04x IO_CLK_DIV0: 0x%04x", 
        IO_CLK_SEL2, IO_CLK_DIV0);
    lcd_putsf(0, line++, " IO_CLK_DIV1: 0x%04x IO_CLK_DIV2: 0x%04x", 
        IO_CLK_DIV1, IO_CLK_DIV2);
    lcd_putsf(0, line++, " IO_CLK_DIV3: 0x%04x IO_CLK_DIV4: 0x%04x", 
        IO_CLK_DIV3, IO_CLK_DIV4);
    lcd_putsf(0, line++, " IO_CLK_BYP : 0x%04x IO_CLK_INV : 0x%04x", 
        IO_CLK_BYP, IO_CLK_INV);
    lcd_putsf(0, line++, " IO_CLK_MOD0: 0x%04x IO_CLK_MOD1: 0x%04x ", 
        IO_CLK_MOD0, IO_CLK_MOD1);
    lcd_putsf(0, line++, " IO_CLK_MOD2: 0x%04x IO_CLK_LPCTL0: 0x%04x ", 
        IO_CLK_MOD2, IO_CLK_LPCTL0);
#endif
    lcd_puts(0, line++, "Interrupt info:");
    lcd_putsf(0, line++, " IO_INTC_EINT0: 0x%04x IO_INTC_EINT1: 0x%04x ", 
        IO_INTC_EINT0, IO_INTC_EINT1);
    lcd_putsf(0, line++, " IO_INTC_EINT2: 0x%04x IO_INTC_IRQ0: 0x%04x ", 
        IO_INTC_EINT2, IO_INTC_IRQ0);
    lcd_putsf(0, line++, " IO_INTC_IRQ1: 0x%04x IO_INTC_IRQ2: 0x%04x ", 
        IO_INTC_IRQ1, IO_INTC_IRQ2);
		
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

#if defined(MROBE_500)
    line++;
#endif
    oldline=line;
    while(!done)
    {
        line = oldline;
#if defined(MROBE_500)
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
#else
        button = button_get(false);
        if(button & BUTTON_POWER)
            done = true;
        else if(button & BUTTON_LEFT)
            lcd_set_direct_fb(false);
        else if(button & BUTTON_RIGHT)
            lcd_set_direct_fb(true);

        lcd_puts(0, line++, "LCD info:");
        lcd_putsf(0, line++, " LCD direct FB access? %s", 
            (lcd_get_direct_fb() ? "yes" : "no"));
        line++;
#endif
        lcd_puts(0, line++, "[Rockbox info]");
        lcd_putsf(0, line++, "current tick: %08x Seconds running: %08d",
            (unsigned int)current_tick, (unsigned int)current_tick/100);
#if defined(MROBE_500)
        lcd_putsf(0, line++, "Address: 0x%08x Data: 0x%08x", 
            (unsigned int)address, *address);
        lcd_putsf(0, line++, "Address: 0x%08x Data: 0x%08x",
            (unsigned int)(address+1), *(address+1));
        lcd_putsf(0, line++, "Address: 0x%08x Data: 0x%08x",
            (unsigned int)(address+2), *(address+2));
#endif

        lcd_update();
    }
    return false;
}
