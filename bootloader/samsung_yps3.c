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
#include "kernel.h"
#include "thread.h"
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

char version[] = APPSVERSION;
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

static int count = 0;

static void timer_callback(void)
{
    count++;
}

void main(void)
{
    char mystring[64];
    int line, col;
    unsigned char read_data[16];
    int i;
    struct si4700_dbg_info si4700_info;
    
    line = 1;

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
    _backlight_init();

    timer_register(1, NULL, 3 * TIMER_FREQ, timer_callback);

    // LED outputs
    PCON3 = (PCON3 & ~(0x0000FF00)) | 0x00001100;
    PDAT3 |= (1 << 2) | (1 << 3);

    // FM power
    si4700_init();
    tuner_power(true);
    si4700_set(RADIO_SLEEP, 0);
    si4700_set(RADIO_MUTE, 0);
    si4700_set(RADIO_FREQUENCY, 100700000);

    lcd_puts_scroll(0,0,"+++ this is a very very long line to test scrolling. ---");

    while (true)
    {
        line = 1;

#if 1   /* enable to see GPIOs */
        snprintf(mystring, 64, "%02X %02X %02X %02X %02X %02X %02X %02X", PDAT0, PDAT1, PDAT2, PDAT3, PDAT4, PDAT5, PDAT6, PDAT7); 
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* enable this to see info about the RTC */
        rtc_read_datetime(read_data);
        snprintf(mystring, 64, "RTC:");
        for (i = 0; i < 7; i++) {
            snprintf(mystring + 2 * i + 4, 64, "%02X", read_data[i]);
        }
        lcd_puts(0, line++, mystring);
#endif 

#if 0   /* enable this so see info about the codec */
        memset(read_data, 0, sizeof(read_data));
        for (i = 0; i < 1; i++) {
            i2c_read(0x34, i, 2, read_data);
            data = read_data[0] << 8 | read_data[1];
            snprintf(mystring + 4 * i, 64, "%04X", data);
        }
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* enable this to see radio debug info */
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

#if 1   /* enable this to see ADC info */
        snprintf(mystring, 64, "%04X %04X %04X %04X", adc_read(0), adc_read(1), adc_read(2), adc_read(3));
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

        snprintf(mystring, 64, "SCR %04X SSR %04X EIR %04X", SCR, SSR, EIR);
        lcd_puts(0, line++, mystring);

        snprintf(mystring, 64, "FAR %04X FNR %04X EP0 %04X", FAR, FNR, EP0SR);
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* enable to see timer/counter info */
        snprintf(mystring, 64, "TIMER: %08X", count);
        lcd_puts(0, line++, mystring);
        snprintf(mystring, 64, "current_tick: %08X", current_tick);
        lcd_puts(0, line++, mystring);
        snprintf(mystring, 64, "TIMER %04X %04X %04X", TDCON, TDPRE, TDDATA0);
        lcd_puts(0, line++, mystring);
#endif

#if 1   /* enable this to control backlight brightness with the hold switch */
        if (PDAT4 & (1 << 3)) {
            _backlight_set_brightness(10);
            PDAT3 &= ~(1 << 4);
        }
        else {
            _backlight_set_brightness(15);
            PDAT3 |= (1 << 4);
        }
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


