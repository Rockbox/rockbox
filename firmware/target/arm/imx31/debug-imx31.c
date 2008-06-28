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
#include "string.h"
#include "button.h"
#include "lcd.h"
#include "sprintf.h"
#include "font.h"
#include "debug-target.h"
#include "mc13783.h"
#include "adc.h"
#include "clkctl-imx31.h"

bool __dbg_hw_info(void)
{
    char buf[50];
    int line;
    unsigned int pllref;
    unsigned int mcu_pllfreq, ser_pllfreq, usb_pllfreq;
    uint32_t mpctl, spctl, upctl;
    unsigned int freq;
    uint32_t regval;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while (1)
    {
        line = 0;
        mpctl = CLKCTL_MPCTL;
        spctl = CLKCTL_SPCTL;
        upctl = CLKCTL_UPCTL;

        pllref = imx31_clkctl_get_pll_ref_clk();

        mcu_pllfreq = imx31_clkctl_get_pll(PLL_MCU);
        ser_pllfreq = imx31_clkctl_get_pll(PLL_SERIAL);
        usb_pllfreq = imx31_clkctl_get_pll(PLL_USB);

        snprintf(buf, sizeof (buf), "pll_ref_clk: %u", pllref);
        lcd_puts(0, line++, buf); line++;

        /* MCU clock domain */
        snprintf(buf, sizeof (buf), "MPCTL: %08lX", mpctl);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof (buf), " mpl_dpdgck_clk: %u", mcu_pllfreq);
        lcd_puts(0, line++, buf); line++;

        regval = CLKCTL_PDR0;
        snprintf(buf, sizeof (buf), "  PDR0: %08lX", regval);
        lcd_puts(0, line++, buf);

        freq = mcu_pllfreq / (((regval & 0x7) + 1));
        snprintf(buf, sizeof (buf), "   mcu_clk:      %u", freq);
        lcd_puts(0, line++, buf);

        freq = mcu_pllfreq / (((regval >> 11) & 0x7) + 1);
        snprintf(buf, sizeof (buf), "   hsp_clk:      %u", freq);
        lcd_puts(0, line++, buf);

        freq = mcu_pllfreq / (((regval >> 3) & 0x7) + 1);
        snprintf(buf, sizeof (buf), "   hclk_clk:     %u", freq);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof (buf), "   ipg_clk:      %u",
            freq / (unsigned)(((regval >> 6) & 0x3) + 1));
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof (buf), "   nfc_clk:      %u",
            freq / (unsigned)(((regval >> 8) & 0x7) + 1));
        lcd_puts(0, line++, buf);

        line++;

        /* Serial clock domain */
        snprintf(buf, sizeof (buf), "SPCTL: %08lX", spctl);
        lcd_puts(0, line++, buf);
        snprintf(buf, sizeof (buf), " spl_dpdgck_clk: %u", ser_pllfreq);
        lcd_puts(0, line++, buf);

        line++;

        /* USB clock domain */
        snprintf(buf, sizeof (buf), "UPCTL: %08lX", upctl);
        lcd_puts(0, line++, buf);

        snprintf(buf, sizeof (buf), " upl_dpdgck_clk: %u", usb_pllfreq);
        lcd_puts(0, line++, buf); line++;

        regval = CLKCTL_PDR1;
        snprintf(buf, sizeof (buf), "  PDR1: %08lX", regval);
        lcd_puts(0, line++, buf);

        freq = usb_pllfreq /
            ((((regval >> 30) & 0x3) + 1) * (((regval >> 27) & 0x7) + 1));
        snprintf(buf, sizeof (buf), "   usb_clk:       %u", freq);
        lcd_puts(0, line++, buf);

        freq = usb_pllfreq / (((CLKCTL_PDR0 >> 16) & 0x1f) + 1);
        snprintf(buf, sizeof (buf), "   ipg_per_baud:  %u", freq);
        lcd_puts(0, line++, buf);
          
        lcd_update();

        if (button_get(true) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }
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
    };

    uint32_t pmic_regs[ARRAYLEN(pmic_regset)];

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

        line++;

        lcd_puts(0, line++, "ADC"); line++;

        for (i = 0; i < NUM_ADC_CHANNELS; i += 4)
        {
            snprintf(buf, sizeof(buf),
                     "CH%02d:%04u CH%02d:%04u CH%02d:%04u CH%02d:%04u",
                     i+0, adc_read(i+0),
                     i+1, adc_read(i+1),
                     i+2, adc_read(i+2),
                     i+3, adc_read(i+3));
            lcd_puts(0, line++, buf);
        }

        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
            return false;
    }
}  
