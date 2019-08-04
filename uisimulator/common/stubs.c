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

#if CONFIG_CODEC != SWCODEC
#include "mp3_playback.h"

void audio_set_buffer_margin(int seconds)
{
     (void)seconds;
}

/* firmware/target/sh/archos/audio-archos.c */

/* list of tracks in memory */
#define MAX_ID3_TAGS (1<<4) /* Must be power of 2 */
#define MAX_ID3_TAGS_MASK (MAX_ID3_TAGS - 1)

static bool paused; /* playback is paused */
static bool playing; /* We are playing an MP3 stream */

bool audio_is_initialized = false;

void mp3_init(int volume, int bass, int treble, int balance, int loudness,
              int avc, int channel_config, int stereo_width, bool swap_channels,
              int mdb_strength, int mdb_harmonics,
              int mdb_center, int mdb_shape, bool mdb_enable,
              bool superbass)
{
    (void)volume;
    (void)bass;
    (void)treble;
    (void)balance;
    (void)loudness;
    (void)avc;
    (void)channel_config;
    (void)stereo_width;
    (void)swap_channels;
    (void)mdb_strength;
    (void)mdb_harmonics;
    (void)mdb_center;
    (void)mdb_shape;
    (void)mdb_enable;
    (void)superbass;
    audio_is_initialized = true;

    playing = false;
    paused = true;
}

void mp3_play_pause(bool play)
{
    (void)play;
}

void mp3_play_stop(void)
{
}

unsigned char* mp3_get_pos(void)
{
    return NULL;
}

void mp3_play_data(const void* start, size_t size,
                   mp3_play_callback_t get_more)
{
    (void)start; (void)size; (void)get_more;
}

void mp3_shutdown(void)
{
}

/* firmware/drivers/audio/mas35xx.c */
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
void audiohw_set_loudness(int value)
{
    (void)value;
}

void audiohw_set_avc(int value)
{
    (void)value;
}

void audiohw_set_mdb_strength(int value)
{
    (void)value;
}

void audiohw_set_mdb_harmonics(int value)
{
    (void)value;
}

void audiohw_set_mdb_center(int value)
{
    (void)value;
}

void audiohw_set_mdb_shape(int value)
{
    (void)value;
}

void audiohw_set_mdb_enable(int value)
{
    (void)value;
}

void audiohw_set_superbass(int value)
{
    (void)value;
}
#endif /* (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F) */
#endif /* CODEC != SWCODEC */

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

#ifdef ARCHOS_PLAYER
bool is_new_player(void)
{
    extern char having_new_lcd;
    return having_new_lcd;
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
