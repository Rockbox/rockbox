/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Björn Stenberg <bjorn@haxx.se>
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
#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include "debug.h"

#include "screens.h"
#include "button.h"

#include "string.h"
#include "lcd.h"

#include "power.h"

#include "ata.h" /* for volume definitions */

static bool storage_spinning = false;

int fat_startsector(void)
{
    return 63;
}

int ata_spinup_time(void)
{
    return 100;
}

int storage_spinup_time(void)
{
    return 0;
}

int storage_init(void)
{
    return 1;
}

int storage_write_sectors(IF_MV(int drive,)
                      unsigned long start,
                      int count,
                      const void* buf)
{
    IF_MV((void)drive;)
    int i;

    for (i=0; i<count; i++ ) {
        FILE* f;
        char name[32];

        sprintf(name,"sector%lX.bin",start+i);
        f=fopen(name,"wb");
        if (f) {
            if(fwrite(buf,512,1,f) != 1) {
                fclose(f);
                return -1;
            }
            fclose(f);
        }
        else {
            return -1;
        }
    }
    return 0;
}

int storage_read_sectors(IF_MV(int drive,)
                     unsigned long start,
                     int count,
                     void* buf)
{
    IF_MV((void)drive;)
    int i;
    size_t ret;

    for (i=0; i<count; i++ ) {
        FILE* f;
        char name[32];

        DEBUGF("Reading sector %lX\n",start+i);
        sprintf(name,"sector%lX.bin",start+i);
        f=fopen(name,"rb");
        if (f) {
            ret = fread(buf,512,1,f);
            fclose(f);
            if (ret != 1)
                return -1;
        }
    }
    return 0;
}

void storage_spin(void)
{
    storage_spinning = true;
}

void storage_sleep(void)
{
}

bool storage_disk_is_active(void)
{
    return storage_spinning;
}

void storage_spindown(int s)
{
    (void)s;
    storage_spinning = false;
}

int rtc_read(int address)
{
    return address ^ 0x55;
}

int rtc_write(int address, int value)
{
    (void)address;
    (void)value;
    return 0;
}

#ifdef HAVE_RTC_ALARM
void rtc_get_alarm(int *h, int *m)
{
    *h = 11;
    *m = 55;
}

void rtc_set_alarm(int h, int m)
{
    (void)h;
    (void)m;
}

void rtc_enable_alarm(bool enable)
{
    (void)enable;
}

extern bool sim_alarm_wakeup;
bool rtc_check_alarm_started(bool release_alarm)
{
    (void)release_alarm;
    return sim_alarm_wakeup;
}

bool rtc_check_alarm_flag(void)
{
    return true;
}
#endif

#ifdef HAVE_HEADPHONE_DETECTION
bool headphones_inserted(void)
{
    return true;
}
#endif

#ifdef HAVE_LINEOUT_DETECTION
bool lineout_inserted(void)
{
    return false;
}
#endif

#ifdef HAVE_SPDIF_POWER
void spdif_power_enable(bool on)
{
   (void)on;
}

bool spdif_powered(void)
{
    return false;
}
#endif

#ifdef HAVE_USB_POWER
bool usb_powered_only(void)
{
    return false;
}

bool usb_charging_enable(bool on)
{
    (void)on;
    return false;
}
#endif

#ifndef USB_NONE
bool usb_inserted(void)
{
    return false;
}
#endif

#ifdef HAVE_REMOTE_LCD_TICKING
void lcd_remote_emireduce(bool state)
{
    (void)state;
}
#endif

void lcd_set_contrast( int x )
{
    (void)x;
}

void mpeg_set_pitch(int pitch)
{
    (void)pitch;
}

void cpu_sleep(bool enabled)
{
    (void)enabled;
}

#ifdef HAVE_TOUCHPAD_SENSITIVITY_SETTING
void touchpad_set_sensitivity(int level)
{
    (void)level;
}
#endif

#if defined(HAVE_TOUCHSCREEN) && !defined(HAS_BUTTON_HOLD)
void touchscreen_enable_device(bool en)
{
    (void)en;
}
#endif

#if defined(HAVE_TOUCHPAD) && !defined(HAS_BUTTON_HOLD)
void touchpad_enable_device(bool en)
{
    (void)en;
}
#endif
