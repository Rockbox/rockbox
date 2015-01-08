/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Bertrik Sikken
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "config.h"

#include "inttypes.h"
#include "cpu.h"
#include "system.h"
#include "lcd.h"
#include "../kernel-internal.h"
#include "backlight.h"
#include "backlight-target.h"
#include "button.h"
#include "panic.h"
#include "power.h"
#include "common.h"
#include "usb.h"
#include "bitmaps/rockboxlogo.h"

#include "adc.h"
#include "adc-target.h"

#include "timer.h"

#include "i2c-s5l8700.h"
#include "dma-target.h"
#include "pcm.h"
#include "audiohw.h"
#include "rtc.h"
#include "tuner.h"
#include "si4700.h"
#include "fmradio_i2c.h"
#include "wmcodec.h"
#include "nand-target.h"

#define LONG_DELAY  200000
#define SHORT_DELAY  50000
#define PAUSE_DELAY  50000

static inline void delay(unsigned int duration)
{
    volatile unsigned int i;
    for(i=0;i<duration;i++);
}

// forward declaration
static int rds_decode(int line, struct si4700_dbg_info *nfo);

void main(void)
{
    char mystring[64];
    int line, col;
    struct tm dt;
    int i;
    struct si4700_dbg_info si4700_info;
    int brightness = DEFAULT_BRIGHTNESS_SETTING;
    unsigned int button;
    unsigned int fm_frequency = 100700000;
    int audiovol = 0x60;
    unsigned nand_ids[4];
    
    // enable all peripherals
    PWRCON = 0;
    
    // disable all interrupts
    INTMSK = 0;
    
    // start with all GPIOs as input
    PCON0 = 0;
    PCON1 = 0;
    PCON2 = 0;
    PCON3 = 0;
    PCON4 = 0;
    PCON5 = 0;
    PCON6 = 0;
    PCON7 = 0;
    
    system_init();
    kernel_init();
    
    asm volatile("msr cpsr_c, #0x13\n\t"); // enable interrupts

    lcd_init();
    
    lcd_clear_display();
    lcd_bitmap(rockboxlogo, 0, 160, BMPWIDTH_rockboxlogo, BMPHEIGHT_rockboxlogo);
    lcd_update();

    power_init();
    
    i2c_init();
    fmradio_i2c_init();
    adc_init();
    backlight_hw_init();
    button_init_device();
        
    // FM power
    si4700_init();
    tuner_power(true);
    si4700_set(RADIO_SLEEP, 0);
    si4700_set(RADIO_MUTE, 0);
    si4700_set(RADIO_REGION, 0);
    si4700_set(RADIO_FREQUENCY, fm_frequency);
    
    lcd_puts_scroll(0,0,"+++ this is a very very long line to test scrolling. ---");

    // WM1800 codec configuration
    wmcodec_write(0x0F, 0);     // codec reset
    wmcodec_write(0x19, 0xF0);  // pwr mgmt1: VMID = 1, VREF, AINL, AINR
    wmcodec_write(0x1A, 0x60);  // pwr mgmt2: LOUT1, ROUT1
    wmcodec_write(0x2F, 0x0C);  // pwr mgmt3: LOMIX, ROMIX
    wmcodec_write(0x02, audiovol);              // LOUT1VOL
    wmcodec_write(0x03, audiovol | (1 << 8));   // ROUT1VOL
    wmcodec_write(0x22, (1 << 7) | (7 << 4));   // left out mix (1)
    wmcodec_write(0x25, (1 << 7) | (7 << 4));   // right out mix (2)
    
    // enable audio
    PCON5 = (PCON5 & ~0x0000000F) | 0x00000001;
    PDAT5 |= 1;

    nand_ll_init();
    for (i = 0; i < 4; i++) {
        nand_ids[i] = nand_ll_read_id(i);
    }

    while (true)
    {
        line = 1;

#if 1   /* enable to see GPIOs */
        snprintf(mystring, 64, "%02X %02X %02X %02X %02X %02X %02X %02X",
            PDAT0, PDAT1, PDAT2, PDAT3, PDAT4, PDAT5, PDAT6, PDAT7);
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* enable this to see info about the RTC */
        rtc_read_datetime(&dt);
        snprintf(mystring, 64, "RTC: %04d-%02d-%02d %02d:%02d:%02d",
            dt.tm_year + 1900, dt.tm_mon+1, dt.tm_mday,
            dt.tm_hour, dt.tm_min, dt.tm_sec);
        lcd_puts(0, line++, mystring);
#endif 

#if 1   /* enable this to see radio debug info */
        button = button_read_device();
        if (button & BUTTON_RIGHT) {
            fm_frequency += 100000;
            si4700_set(RADIO_FREQUENCY, fm_frequency);
        }
        if (button & BUTTON_LEFT) {
            fm_frequency -= 100000;
            si4700_set(RADIO_FREQUENCY, fm_frequency);
        }
        snprintf(mystring, 64, "FM frequency: %9d", fm_frequency);
        lcd_puts(0, line++, mystring);

        si4700_dbg_info(&si4700_info);
        col = snprintf(mystring, 64, "FM: ");
        for (i = 0; i < 16; i++) {
            col += snprintf(mystring + col, 64, "%04X ", si4700_info.regs[i]);
            if (((i + 1) % 4) == 0) {
                lcd_puts(0, line++, mystring);
                col = 4;
            }
        }
        line = rds_decode(line, &si4700_info);
#endif

#if 1   /* volume control with up/down keys */
        button = button_read_device();
        if (button & BUTTON_UP) {
            if (audiovol < 127) {
                audiovol++;
                wmcodec_write(0x02, audiovol);
                wmcodec_write(0x03, (1 << 8) | audiovol);
            }
        }
        if (button & BUTTON_DOWN) {
            if (audiovol > 0) {
                audiovol--;
                wmcodec_write(0x02, audiovol);
                wmcodec_write(0x03, (1 << 8) | audiovol);
            }
        }
        snprintf(mystring, 64, "volume %3d", audiovol);
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* enable this to see ADC info */
        snprintf(mystring, 64, "ADC: %04X %04X %04X %04X", 
            adc_read(0), adc_read(1), adc_read(2), adc_read(3));
        lcd_puts(0, line++, mystring);
        snprintf(mystring, 64, "ADC:USB %4d mV BAT %4d mV",
            (adc_read(0) * 6000) >> 10, (adc_read(2) * 4650) >> 10);
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* enable this so see USB info */
        snprintf(mystring, 64, "CLK %08X CLK2 %08X", CLKCON, CLKCON2);
        lcd_puts(0, line++, mystring);

        snprintf(mystring, 64, "%04X %04X %04X %04X", PHYCTRL, PHYPWR, URSTCON, UCLKCON);
        lcd_puts(0, line++, mystring);

        snprintf(mystring, 64, "SCR %04X SSR %04X EIR %04X", USB_SCR, USB_SSR, USB_EIR);
        lcd_puts(0, line++, mystring);

        snprintf(mystring, 64, "FAR %04X FNR %04X EP0 %04X", USB_FAR, USB_FNR, USB_EP0SR);
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* button lights controlled by keypad */
        button = button_read_device();
        if (button & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT)) {
            PDAT3 |= (1 << 3);
        }
        else {
            PDAT3 &= ~(1 << 3); 
        }
        if (button & (BUTTON_BACK | BUTTON_MENU)) {
            PDAT3 |= (1 << 2);
        }
        else {
            PDAT3 &= ~(1 << 2);
        }
        if (button & (BUTTON_SELECT)) {
            PDAT4 |= (1 << 2);
        }
        else {
            PDAT4 &= ~(1 << 2);
        }
#endif

#if 1   /* backlight brightness controlled by up/down keys */
        button = button_read_device();
        if (button & BUTTON_MENU) {
            if (brightness < MAX_BRIGHTNESS_SETTING) {
                brightness++;
                backlight_hw_brightness(brightness);
            }
        }
        else  if (button & BUTTON_BACK) {
            if (brightness > MIN_BRIGHTNESS_SETTING) {
                brightness--;
                backlight_hw_brightness(brightness);
            }
        }
        snprintf(mystring, 64, "brightness %3d", brightness);
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* power off using power button */
        button = button_read_device();
        if (button & BUTTON_POWER) {
            power_off();
        }
#endif

#if 1   /* button info */
        snprintf(mystring, 64, "BUTTONS %08X, %s", button_read_device(),
                 headphones_inserted() ? "HP" : "hp");
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* NAND debug */
        snprintf(mystring, 64, "NAND ID: %08X %08X", nand_ids[0], nand_ids[1]);
        lcd_puts(0, line++, mystring);
        snprintf(mystring, 64, "NAND ID: %08X %08X", nand_ids[2], nand_ids[3]);
        lcd_puts(0, line++, mystring);
#endif

#if 1
        snprintf(mystring, 64, "TIMER A:%08X B:%08X", TACNT, TBCNT);
        lcd_puts(0, line++, mystring);
        snprintf(mystring, 64, "TIMER C:%08X D:%08X", TCCNT, TDCNT);
        lcd_puts(0, line++, mystring);
#endif

        lcd_update();
    }
}


static int rds_decode(int line, struct si4700_dbg_info *nfo)
{
    unsigned short rdsdata[4];
    unsigned int pi, group, tp, pty, segment, abflag;
    static unsigned int af1 = 0, af2 = 0;
    static unsigned int day = 0, hour = 0, minute = 0;
    static unsigned int abflag_prev = -1;
    static char mystring[64];
    
    /* big RDS arrays */
    static char ps[9];
    static char rt[65];
    
    rdsdata[0] = nfo->regs[12];
    rdsdata[1] = nfo->regs[13];
    rdsdata[2] = nfo->regs[14];
    rdsdata[3] = nfo->regs[15];

    pi = rdsdata[0];
    group = (rdsdata[1] >> 11) & 0x1F;
    tp = (rdsdata[1] >> 10) & 1;
    pty = (rdsdata[1] >> 5) & 0x1F;
    
    switch (group) {
    
    case 0: /* group 0A: basic info */
        af1 = (rdsdata[2] >> 8) & 0xFF;
        af2 = (rdsdata[2] >> 0) & 0xFF;
        /* fall through */
    case 1: /* group 0B: basic info */
        segment = rdsdata[1] & 3;
        ps[segment * 2 + 0]  = (rdsdata[3] >> 8) & 0xFF;
        ps[segment * 2 + 1]  = (rdsdata[3] >> 0) & 0xFF;
        break;
    
    case 2: /* group 1A: programme item */
    case 3: /* group 1B: programme item */
        day = (rdsdata[3] >> 11) & 0x1F;
        hour = (rdsdata[3] >> 6) & 0x1F;
        minute = (rdsdata[3] >> 0) & 0x3F;
        break;
        
    case 4: /* group 2A: radio text */
        segment = rdsdata[1] & 0xF;
        abflag = (rdsdata[1] >> 4) & 1;
        if (abflag != abflag_prev) {
            memset(rt, '.', 64);
            abflag_prev = abflag;
        }
        rt[segment * 4 + 0] = (rdsdata[2] >> 8) & 0xFF;
        rt[segment * 4 + 1] = (rdsdata[2] >> 0) & 0xFF;
        rt[segment * 4 + 2] = (rdsdata[3] >> 8) & 0xFF;
        rt[segment * 4 + 3] = (rdsdata[3] >> 0) & 0xFF;
        break;
        
    case 5: /* group 2B: radio text */
        segment = rdsdata[1] & 0xF;
        abflag = (rdsdata[1] >> 4) & 1;
        if (abflag != abflag_prev) {
            memset(rt, '.', 64);
            abflag_prev = abflag;
        }
        rt[segment * 2 + 0] = (rdsdata[3] >> 8) & 0xFF;
        rt[segment * 2 + 1] = (rdsdata[3] >> 0) & 0xFF;
        break;
    
    default:
        break;
    }

    snprintf(mystring, 64, "PI:%04X,TP:%d,PTY:%2d,AF:%02X/%02X", pi, tp, pty, af1, af2);
    lcd_puts(0, line++, mystring);
    snprintf(mystring, 64, "PS:%s,ITEM:%02d-%02d:%02d", ps, day, hour, minute);
    lcd_puts(0, line++, mystring);
    snprintf(mystring, 64, "RT:%s", rt);
    lcd_puts(0, line++, mystring);
    
    return line;
}


