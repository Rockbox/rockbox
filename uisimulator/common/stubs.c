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

extern char having_new_lcd;
static bool storage_spinning = false;

#if CONFIG_CODEC != SWCODEC
void audio_set_buffer_margin(int seconds)
{
     (void)seconds;
}
#endif

int fat_startsector(void)
{
    return 63;
}

bool fat_ismounted(int volume)
{
    (void)volume;
    return true;
}

int storage_init(void)
{
    return 1;
}

int storage_write_sectors(IF_MV2(int drive,)
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
            fwrite(buf,512,1,f);
            fclose(f);
        }
    }
    return 1;
}

int storage_read_sectors(IF_MV2(int drive,)
                     unsigned long start,
                     int count,
                     void* buf)
{
    IF_MV((void)drive;)
    int i;

    for (i=0; i<count; i++ ) {
        FILE* f;
        char name[32];

        DEBUGF("Reading sector %lX\n",start+i);
        sprintf(name,"sector%lX.bin",start+i);
        f=fopen(name,"rb");
        if (f) {
            fread(buf,512,1,f);
            fclose(f);
        }
    }
    return 1;
}

void storage_spin(void)
{
    storage_spinning = true;
}

void storage_sleep(void)
{
    DEBUGF("storage_sleep()\n");
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

void rtc_init(void)
{
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

int rtc_read_datetime(unsigned char* buf)
{
    time_t now = time(NULL);
    struct tm *teem = localtime(&now);

    buf[0] = (teem->tm_sec%10) | ((teem->tm_sec/10) << 4);
    buf[1] = (teem->tm_min%10) | ((teem->tm_min/10) << 4);
    buf[2] = (teem->tm_hour%10) | ((teem->tm_hour/10) << 4);
    buf[3] = (teem->tm_wday);
    buf[4] = (teem->tm_mday%10) | ((teem->tm_mday/10) << 4);
    buf[5] = ((teem->tm_year-100)%10) | (((teem->tm_year-100)/10) << 4);
    buf[6] = ((teem->tm_mon+1)%10) | (((teem->tm_mon+1)/10) << 4);

    return 0;
}

int rtc_write_datetime(unsigned char* buf)
{
    (void)buf;
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

bool rtc_enable_alarm(bool enable)
{
    return enable;
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

#ifdef HAVE_LCD_ENABLE
bool lcd_enabled(void)
{
    return true;
}
#endif

bool charging_state(void)
{
    return false;
}

unsigned int power_input_status(void)
{
#ifdef HAVE_BATTERY_SWITCH
    return POWER_INPUT_BATTERY;
#else
    return POWER_INPUT_NONE;
#endif
}

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

bool is_new_player(void)
{
    return having_new_lcd;
}

#ifdef HAVE_USB_POWER
bool usb_powered(void)
{
    return false;
}

#if CONFIG_CHARGING
bool charger_inserted(void)
{
    return false;
}

bool usb_charging_enable(bool on)
{
    (void)on;
    return false;
}
#endif
#endif

bool usb_inserted(void)
{
    return false;
}

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

static int sleeptime;
void set_sleep_timer(int seconds)
{
    sleeptime = seconds;
}

int get_sleep_timer(void)
{
    return sleeptime;
}

#ifdef HAVE_LCD_CHARCELLS
void lcd_clearrect (int x, int y, int nx, int ny)
{
  /* Reprint char if you want to change anything */
  (void)x;
  (void)y;
  (void)nx;
  (void)ny;
}

void lcd_fillrect (int x, int y, int nx, int ny)
{
  /* Reprint char if you want to change display anything */
  (void)x;
  (void)y;
  (void)nx;
  (void)ny;
}
#endif

void cpu_sleep(bool enabled)
{
    (void)enabled;
}

void button_set_flip(bool yesno)
{
    (void)yesno;
}

#ifdef HAVE_TOUCHPAD_SENSITIVITY_SETTING
void touchpad_set_sensitivity(int level)
{
    (void)level;
}
#endif

/* assure an unused place to direct virtual pointers to */
#define VIRT_SIZE 0xFFFF /* more than enough for our string ID range */
unsigned char vp_dummy[VIRT_SIZE];

