/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "system.h"
#include "string.h"
#include "button.h"
#include "lcd.h"
#include "sprintf.h"
#include "font.h"
#include "debug-target.h"
#include "mc13783.h"

bool __dbg_hw_info(void)
{
    return false;
}

bool __dbg_ports(void)
{
    char buf[50];
    int line;
    int i;

    static const char pmic_regset[] =
    {
        MC13783_INTERRUPT_STATUS0,
        MC13783_INTERRUPT_SENSE0,
        MC13783_INTERRUPT_STATUS1,
        MC13783_INTERRUPT_SENSE1,
        MC13783_RTC_TIME,
        MC13783_RTC_ALARM,
        MC13783_RTC_DAY,
        MC13783_RTC_DAY_ALARM,
        MC13783_ADC0,
        MC13783_ADC1,
        MC13783_ADC2,
        MC13783_ADC3,
        MC13783_ADC4,
    };

    static const char *pmic_regnames[ARRAYLEN(pmic_regset)] =
    {
        "Int Stat0 ",
        "Int Sense0",
        "Int Stat1 ",
        "Int Sense1",
        "RTC Time  ",
        "RTC Alarm ",
        "RTC Day   ",
        "RTC Day Al",
        "ADC0      ",
        "ADC1      ",
        "ADC2      ",
        "ADC3      ",
        "ADC4      ",
    };

    uint32_t pmic_regs[ARRAYLEN(pmic_regset)];

    lcd_setmargins(0, 0);
    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
        snprintf(buf, sizeof(buf), "[Ports and Registers]");
        lcd_puts(0, line++, buf); line++;

        /* GPIO1 */
        snprintf(buf, sizeof(buf), "GPIO1: DR:   %08lx GDIR: %08lx", GPIO1_DR, GPIO1_GDIR);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       PSR:  %08lx ICR1: %08lx", GPIO1_PSR, GPIO1_ICR1);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       ICR2: %08lx IMR:  %08lx", GPIO1_ICR2, GPIO1_IMR);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       ISR:  %08lx",             GPIO1_ISR);
        lcd_puts(0, line++, buf); line++;

        /* GPIO2 */
        snprintf(buf, sizeof(buf), "GPIO2: DR:   %08lx GDIR: %08lx", GPIO2_DR, GPIO2_GDIR);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       PSR:  %08lx ICR1: %08lx", GPIO2_PSR, GPIO2_ICR1);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       ICR2: %08lx IMR:  %08lx", GPIO2_ICR2, GPIO2_IMR);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       ISR:  %08lx",             GPIO2_ISR);
        lcd_puts(0, line++, buf); line++;

        /* GPIO3 */
        snprintf(buf, sizeof(buf), "GPIO3: DR:   %08lx GDIR: %08lx", GPIO3_DR, GPIO3_GDIR);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       PSR:  %08lx ICR1: %08lx", GPIO3_PSR, GPIO3_ICR1);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       ICR2: %08lx IMR:  %08lx", GPIO3_ICR2, GPIO3_IMR);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof(buf), "       ISR:  %08lx",             GPIO3_ISR);
        lcd_puts(0, line++, buf); line++;

        lcd_puts(0, line++, "PMIC Registers"); line++;

        mc13783_read_regset(pmic_regset, pmic_regs, ARRAYLEN(pmic_regs));

        for (i = 0; i < (int)ARRAYLEN(pmic_regs); i++)
        {
            snprintf(buf, sizeof(buf), "%s: %08lx", pmic_regnames[i], pmic_regs[i]);
            lcd_puts(0, line++, buf);
        }

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }
}  
