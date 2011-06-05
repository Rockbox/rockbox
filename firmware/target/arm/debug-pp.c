/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Dave Chapman
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
#include <stdbool.h>
#include "font.h"
#include "lcd.h"
#include "button.h"
#include "powermgmt.h"
#include "adc.h"
#include "iap.h"
#include "hwcompat.h"
#include "debug-target.h"

static int perfcheck(void)
{
    int result;

    asm (
        "mrs     r2, CPSR            \n"
        "orr     r0, r2, #0xc0       \n" /* disable IRQ and FIQ */
        "msr     CPSR_c, r0          \n"
        "mov     %[res], #0          \n"
        "ldr     r0, [%[timr]]       \n"
        "add     r0, r0, %[tmo]      \n"
    "1:                              \n"
        "add     %[res], %[res], #1  \n"
        "ldr     r1, [%[timr]]       \n"
        "cmp     r1, r0              \n"
        "bmi     1b                  \n"
        "msr     CPSR_c, r2          \n" /* reset IRQ and FIQ state */
        :
        [res]"=&r"(result)
        :
        [timr]"r"(&USEC_TIMER),
        [tmo]"r"(
#if CONFIG_CPU == PP5002
        16000
#else /* PP5020/5022/5024 */
        10226
#endif
        )
        :
        "r0", "r1", "r2"
    );
    return result;
}

bool dbg_ports(void)
{
    int line;

    lcd_clear_display();
    lcd_setfont(FONT_SYSFIXED);

    while(1)
    {
        line = 0;
#if defined(CPU_PP502x)
#if (LCD_HEIGHT >= 176) /* Only for displays with appropriate height. */
        lcd_puts(0, line++, "GPIO ENABLE:");
        lcd_putsf(0, line++, "A: %02x  E: %02x  I: %02x",
                               (unsigned int)GPIOA_ENABLE,
                               (unsigned int)GPIOE_ENABLE,
                               (unsigned int)GPIOI_ENABLE);
        lcd_putsf(0, line++, "B: %02x  F: %02x  J: %02x",
                               (unsigned int)GPIOB_ENABLE,
                               (unsigned int)GPIOF_ENABLE,
                               (unsigned int)GPIOJ_ENABLE);
        lcd_putsf(0, line++, "C: %02x  G: %02x  K: %02x",
                               (unsigned int)GPIOC_ENABLE,
                               (unsigned int)GPIOG_ENABLE,
                               (unsigned int)GPIOK_ENABLE);
        lcd_putsf(0, line++, "D: %02x  H: %02x  L: %02x",
                               (unsigned int)GPIOD_ENABLE,
                               (unsigned int)GPIOH_ENABLE,
                               (unsigned int)GPIOL_ENABLE);
        line++;
#endif
        lcd_puts(0, line++, "GPIO INPUT VAL:");
        lcd_putsf(0, line++, "A: %02x  E: %02x  I: %02x",
                               (unsigned int)GPIOA_INPUT_VAL,
                               (unsigned int)GPIOE_INPUT_VAL,
                               (unsigned int)GPIOI_INPUT_VAL);
        lcd_putsf(0, line++, "B: %02x  F: %02x  J: %02x",
                               (unsigned int)GPIOB_INPUT_VAL,
                               (unsigned int)GPIOF_INPUT_VAL,
                               (unsigned int)GPIOJ_INPUT_VAL);
        lcd_putsf(0, line++, "C: %02x  G: %02x  K: %02x",
                               (unsigned int)GPIOC_INPUT_VAL,
                               (unsigned int)GPIOG_INPUT_VAL,
                               (unsigned int)GPIOK_INPUT_VAL);
        lcd_putsf(0, line++, "D: %02x  H: %02x  L: %02x",
                               (unsigned int)GPIOD_INPUT_VAL,
                               (unsigned int)GPIOH_INPUT_VAL,
                               (unsigned int)GPIOL_INPUT_VAL);
        line++;
        lcd_putsf(0, line++, "GPO32_VAL: %08lx", GPO32_VAL);
        lcd_putsf(0, line++, "GPO32_EN:  %08lx", GPO32_ENABLE);
        lcd_putsf(0, line++, "DEV_EN:    %08lx", DEV_EN);
        lcd_putsf(0, line++, "DEV_EN2:   %08lx", DEV_EN2);
        lcd_putsf(0, line++, "DEV_EN3:   %08lx", inl(0x60006044)); /* to be verified */
        lcd_putsf(0, line++, "DEV_INIT1: %08lx", DEV_INIT1);
        lcd_putsf(0, line++, "DEV_INIT2: %08lx", DEV_INIT2);
#ifdef ADC_ACCESSORY
        lcd_putsf(0, line++, "ACCESSORY: %d", adc_read(ADC_ACCESSORY));
#endif
#ifdef IPOD_VIDEO
        lcd_putsf(0, line++, "4066_ISTAT: %d", adc_read(ADC_4066_ISTAT));
#endif

#if defined(IPOD_ACCESSORY_PROTOCOL)
        const unsigned char *serbuf = iap_get_serbuf();
        lcd_putsf(0, line++, "IAP PACKET: %02x %02x %02x %02x %02x %02x %02x %02x", 
         serbuf[0], serbuf[1], serbuf[2], serbuf[3], serbuf[4], serbuf[5],
         serbuf[6], serbuf[7]);
#endif

#if defined(IRIVER_H10) || defined(IRIVER_H10_5GB)
        line++;
        lcd_putsf(0, line++, "BATT: %03x UNK1: %03x",
                                adc_read(ADC_BATTERY), adc_read(ADC_UNKNOWN_1));
        lcd_putsf(0, line++, "REM:  %03x PAD: %03x",
                                 adc_read(ADC_REMOTE), adc_read(ADC_SCROLLPAD));
#elif defined(PHILIPS_HDD1630) || defined(PHILIPS_HDD6330)
        line++;
        lcd_putsf(0, line++, "BATT: %03x UNK1: %03x",
                                adc_read(ADC_BATTERY), adc_read(ADC_UNKNOWN_1));
#elif defined(SANSA_E200) || defined(PHILIPS_SA9200)
        lcd_putsf(0, line++, "ADC_BVDD:     %4d", adc_read(ADC_BVDD));
        lcd_putsf(0, line++, "ADC_RTCSUP:   %4d", adc_read(ADC_RTCSUP));
        lcd_putsf(0, line++, "ADC_UVDD:     %4d", adc_read(ADC_UVDD));
        lcd_putsf(0, line++, "ADC_CHG_IN:   %4d", adc_read(ADC_CHG_IN));
        lcd_putsf(0, line++, "ADC_CVDD:     %4d", adc_read(ADC_CVDD));
        lcd_putsf(0, line++, "ADC_BATTEMP:  %4d", adc_read(ADC_BATTEMP));
        lcd_putsf(0, line++, "ADC_MICSUP1:  %4d", adc_read(ADC_MICSUP1));
        lcd_putsf(0, line++, "ADC_MICSUP2:  %4d", adc_read(ADC_MICSUP2));
        lcd_putsf(0, line++, "ADC_VBE1:     %4d", adc_read(ADC_VBE1));
        lcd_putsf(0, line++, "ADC_VBE2:     %4d", adc_read(ADC_VBE2));
        lcd_putsf(0, line++, "ADC_I_MICSUP1:%4d", adc_read(ADC_I_MICSUP1));
#if !defined(PHILIPS_SA9200)
        lcd_putsf(0, line++, "ADC_I_MICSUP2:%4d", adc_read(ADC_I_MICSUP2));
        lcd_putsf(0, line++, "ADC_VBAT:     %4d", adc_read(ADC_VBAT));
#endif
#endif

#elif CONFIG_CPU == PP5002
        lcd_putsf(0, line++, "GPIO_A: %02x GPIO_B: %02x",
                 (unsigned int)GPIOA_INPUT_VAL, (unsigned int)GPIOB_INPUT_VAL);
        lcd_putsf(0, line++, "GPIO_C: %02x GPIO_D: %02x",
                 (unsigned int)GPIOC_INPUT_VAL, (unsigned int)GPIOD_INPUT_VAL);

        lcd_putsf(0, line++, "DEV_EN:       %08lx", DEV_EN);
        lcd_putsf(0, line++, "CLOCK_ENABLE: %08lx", CLOCK_ENABLE);
        lcd_putsf(0, line++, "CLOCK_SOURCE: %08lx", CLOCK_SOURCE);
        lcd_putsf(0, line++, "PLL_CONTROL:  %08lx", PLL_CONTROL);
        lcd_putsf(0, line++, "PLL_DIV:      %08lx", PLL_DIV);
        lcd_putsf(0, line++, "PLL_MULT:     %08lx", PLL_MULT);
        lcd_putsf(0, line++, "TIMING1_CTL:  %08lx", TIMING1_CTL);
        lcd_putsf(0, line++, "TIMING2_CTL:  %08lx", TIMING2_CTL);
#endif
        lcd_update();
        if (button_get_w_tmo(HZ/10) == (DEBUG_CANCEL|BUTTON_REL))
        {
            lcd_setfont(FONT_UI);
            return false;
        }
    }
    return false;
}

bool dbg_hw_info(void)
{
    int line = 0;
#if defined(CPU_PP502x)
    char pp_version[] = { (PP_VER2 >> 24) & 0xff, (PP_VER2 >> 16) & 0xff,
                          (PP_VER2 >> 8) & 0xff, (PP_VER2) & 0xff,
                          (PP_VER1 >> 24) & 0xff, (PP_VER1 >> 16) & 0xff,
                          (PP_VER1 >> 8) & 0xff, (PP_VER1) & 0xff, '\0' };
#elif CONFIG_CPU == PP5002
    char pp_version[] = { (PP_VER4 >> 8) & 0xff, PP_VER4 & 0xff,
                          (PP_VER3 >> 8) & 0xff, PP_VER3 & 0xff,
                          (PP_VER2 >> 8) & 0xff, PP_VER2 & 0xff,
                          (PP_VER1 >> 8) & 0xff, PP_VER1 & 0xff, '\0' };
#endif

    lcd_setfont(FONT_SYSFIXED);
    lcd_clear_display();

    lcd_puts(0, line++, "[Hardware info]");

#ifdef IPOD_ARCH
    lcd_putsf(0, line++, "HW rev: 0x%08lx", IPOD_HW_REVISION);
#endif

#ifdef IPOD_COLOR
    extern int lcd_type; /* Defined in lcd-colornano.c */

    lcd_putsf(0, line++, "LCD type: %d", lcd_type);
#endif

    lcd_putsf(0, line++, "PP version: %s", pp_version);

    lcd_putsf(0, line++, "Est. clock (kHz): %d", perfcheck());

    lcd_update();

    /* wait for exit */
    while (button_get_w_tmo(HZ/10) != (DEBUG_CANCEL|BUTTON_REL));

    lcd_setfont(FONT_UI);
    return false;
}
