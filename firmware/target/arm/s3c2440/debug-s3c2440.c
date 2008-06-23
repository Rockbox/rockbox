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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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

bool __dbg_hw_info(void)
{
    return false;
}

bool __dbg_ports(void)
{
    char buf[50];
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        snprintf(buf, sizeof(buf), "[Ports and Registers]");                        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPACON: %08x GPBCON: %08x", GPACON, GPBCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPADAT: %08x GPBDAT: %08x", GPADAT, GPBDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPAUP:  %08x GPBUP:  %08x", 0, GPBUP);          lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCCON: %08x GPDCON: %08x", GPCCON, GPDCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCDAT: %08x GPDDAT: %08x", GPCDAT, GPDDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCUP:  %08x GPDUP:  %08x", GPCUP, GPDUP);      lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPCCON: %08x GPDCON: %08x", GPCCON, GPDCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCDAT: %08x GPDDAT: %08x", GPCDAT, GPDDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPCUP:  %08x GPDUP:  %08x", GPCUP, GPDUP);      lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPECON: %08x GPFCON: %08x", GPECON, GPFCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPEDAT: %08x GPFDAT: %08x", GPEDAT, GPFDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPEUP:  %08x GPFUP:  %08x", GPEUP, GPFUP);      lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPGCON: %08x GPHCON: %08x", GPGCON, GPHCON);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPGDAT: %08x GPHDAT: %08x", GPGDAT, GPHDAT);    lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPGUP:  %08x GPHUP:  %08x", GPGUP, GPHUP);      lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "GPJCON: %08x", GPJCON);                         lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPJDAT: %08x", GPJDAT);                         lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "GPJUP:  %08x", GPJUP);                          lcd_puts(0, line++, buf);
        
        line++;

        snprintf(buf, sizeof(buf), "SRCPND:  %08x INTMOD:  %08x", SRCPND, INTMOD);  lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "INTMSK:  %08x INTPND:  %08x", INTMSK, INTPND);  lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CLKCON:  %08x CLKSLOW: %08x", CLKCON, CLKSLOW); lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "MPLLCON: %08x UPLLCON: %08x", MPLLCON, UPLLCON); lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "CLKDIVN: %08x CAMDIVN: %08x", CLKDIVN, CAMDIVN); lcd_puts(0, line++, buf);
        snprintf(buf, sizeof(buf), "BWSCON:  %08x TCONSEL: %08x", BWSCON, TCONSEL);  lcd_puts(0, line++, buf);

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }
}  
