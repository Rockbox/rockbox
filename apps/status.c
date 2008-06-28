/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Linus Nielsen Feltzing
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
#include "string.h"
#include "lcd.h"
#include "debug.h"
#include "kernel.h"
#include "power.h"
#include "thread.h"
#include "settings.h"
#include "status.h"
#include "mp3_playback.h"
#include "audio.h"
#include "gwps.h"
#include "abrepeat.h"
#include "statusbar.h"
#if CONFIG_RTC
#include "timefuncs.h"
#endif
#ifdef HAVE_LCD_BITMAP
#include "icons.h"
#include "font.h"
#endif
#include "powermgmt.h"
#include "led.h"
#include "sound.h"
#if CONFIG_KEYPAD == IRIVER_H100_PAD
#include "button.h"
#endif
#include "usb.h"
#if CONFIG_TUNER
#include "radio.h"
#endif
#if defined(HAVE_RECORDING) && CONFIG_CODEC == SWCODEC
#include "pcm_record.h"
#endif

static enum playmode ff_mode;

void status_init(void)
{
    ff_mode = 0;
}

void status_set_ffmode(enum playmode mode)
{
    ff_mode = mode; /* Either STATUS_FASTFORWARD or STATUS_FASTBACKWARD */
}

enum playmode status_get_ffmode(void)
{
    /* only use this function for STATUS_FASTFORWARD or STATUS_FASTBACKWARD */
    /* use audio_status() for other modes */
    return ff_mode;
}

int current_playmode(void)
{
    int audio_stat = audio_status();

    /* ff_mode can be either STATUS_FASTFORWARD or STATUS_FASTBACKWARD
       and that supercedes the other modes */
    if(ff_mode)
        return ff_mode;
    
    if(audio_stat & AUDIO_STATUS_PLAY)
    {
        if(audio_stat & AUDIO_STATUS_PAUSE)
            return STATUS_PAUSE;
        else
            return STATUS_PLAY;
    }

#ifdef HAVE_RECORDING
    if(audio_stat & AUDIO_STATUS_RECORD)
    {
        if(audio_stat & AUDIO_STATUS_PAUSE)
            return STATUS_RECORD_PAUSE;
        else
            return STATUS_RECORD;
    }
#endif

#if CONFIG_TUNER
    audio_stat = get_radio_status();
    if(audio_stat & FMRADIO_PLAYING)
       return STATUS_RADIO;

    if(audio_stat & FMRADIO_PAUSED)
       return STATUS_RADIO_PAUSE;
#endif
    
    return STATUS_STOP;
}

#if defined(HAVE_LCD_CHARCELLS)
bool record = false;
bool audio = false;
bool param = false;
bool usb = false;

void status_set_record(bool b)
{
    record = b;
}

void status_set_audio(bool b)
{
    audio = b;
}

void status_set_param(bool b)
{
    param = b;
}

void status_set_usb(bool b)
{
    usb = b;
}

#endif /* HAVE_LCD_CHARCELLS */
