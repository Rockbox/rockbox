/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Robert E. Hak
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
#ifndef _ICONS_H_
#define _ICONS_H_

#ifndef PLUGIN

#include <lcd.h>

#ifdef HAVE_LCD_BITMAP

/* External bitmaps */

#include <rockboxlogo.h>
#ifdef HAVE_REMOTE_LCD
#include <remote_rockboxlogo.h>
#endif


/* Symbolic names for icons */
enum icons_5x8 {
    Icon_Lock_Main,
    Icon_Lock_Remote,
    Icon_Stereo,
    Icon_Mono,
#if CONFIG_CODEC != SWCODEC
    Icon_q,
#endif
    Icon5x8Last
};

enum icons_7x8 {
    Icon_Plug,
    Icon_USBPlug,
    Icon_Mute,
    Icon_Play,
    Icon_Stop,
    Icon_Pause,
    Icon_FastForward,
    Icon_FastBackward,
    Icon_Record,
    Icon_RecPause,
    Icon_Radio,
    Icon_Radio_Mute,
    Icon_Repeat,
    Icon_RepeatOne,
    Icon_Shuffle,
    Icon_DownArrow,
    Icon_UpArrow,
    Icon_RepeatAB,
    Icon7x8Last
};

#if CONFIG_CODEC == SWCODEC && defined (HAVE_RECORDING)
#define BM_GLYPH_WIDTH 4
enum Glyphs_4x8 {
    Glyph_4x8_0 = 0,
    Glyph_4x8_1,
    Glyph_4x8_2,
    Glyph_4x8_3,
    Glyph_4x8_4,
    Glyph_4x8_5,
    Glyph_4x8_6,
    Glyph_4x8_7,
    Glyph_4x8_8,
    Glyph_4x8_9,
    Glyph_4x8_k,
    Glyph_4x8Last
};
extern const unsigned char bitmap_glyphs_4x8[Glyph_4x8Last][4];

#define BM_MPA_L3_M_WIDTH 6
#ifdef ID3_H
/* This enum is redundant but sort of in keeping with the style */
enum rec_format_18x8 {
    Format_18x8_AIFF    = REC_FORMAT_AIFF,
    Format_18x8_MPA_L3  = REC_FORMAT_MPA_L3,
    Format_18x8_WAVPACK = REC_FORMAT_WAVPACK,
    Format_18x8_PCM_WAV = REC_FORMAT_PCM_WAV,
    Format_18x8Last     = REC_NUM_FORMATS
};
extern const unsigned char bitmap_formats_18x8[Format_18x8Last][18];
#endif /* ID3_H */
#endif /* CONFIG_CODEC == SWCODEC && defined (HAVE_RECORDING) */

extern const unsigned char bitmap_icons_5x8[Icon5x8Last][5];
extern const unsigned char bitmap_icons_7x8[Icon7x8Last][7];
extern const unsigned char bitmap_icon_disk[];

#define STATUSBAR_X_POS       0
#define STATUSBAR_Y_POS       0 /* MUST be a multiple of 8 */
#define STATUSBAR_HEIGHT      8
#define STATUSBAR_WIDTH       LCD_WIDTH
#define ICON_BATTERY_X_POS    0
#define ICON_BATTERY_WIDTH    18
#define ICON_PLUG_X_POS       STATUSBAR_X_POS+ICON_BATTERY_WIDTH+2
#define ICON_PLUG_WIDTH       7
#define ICON_VOLUME_X_POS     STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+2+2
#define ICON_VOLUME_WIDTH     16
#define ICON_PLAY_STATE_X_POS STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+ICON_VOLUME_WIDTH+2+2+2
#define ICON_PLAY_STATE_WIDTH 7
#define ICON_PLAY_MODE_X_POS  STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+ICON_VOLUME_WIDTH+ICON_PLAY_STATE_WIDTH+2+2+2+2
#define ICON_PLAY_MODE_WIDTH  7
#define ICON_SHUFFLE_X_POS    STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+ICON_VOLUME_WIDTH+ICON_PLAY_STATE_WIDTH+ICON_PLAY_MODE_WIDTH+2+2+2+2+2
#define ICON_SHUFFLE_WIDTH    7
#define LOCK_X_POS            STATUSBAR_X_POS+ICON_BATTERY_WIDTH+ICON_PLUG_WIDTH+ICON_VOLUME_WIDTH+ICON_PLAY_STATE_WIDTH+ICON_PLAY_MODE_WIDTH+ICON_SHUFFLE_WIDTH+2+2+2+2+2+2
#define LOCK_WIDTH            5
#define ICON_DISK_WIDTH       12
#define ICON_DISK_X_POS       STATUSBAR_WIDTH-ICON_DISK_WIDTH
#define TIME_X_END            STATUSBAR_WIDTH-1

extern void statusbar_wipe(void);
extern void statusbar_icon_battery(int percent);
extern bool statusbar_icon_volume(int percent);
extern void statusbar_icon_play_state(int state);
extern void statusbar_icon_play_mode(int mode);
extern void statusbar_icon_shuffle(void);
extern void statusbar_icon_lock(void);
#if CONFIG_RTC
extern void statusbar_time(int hour, int minute);
#endif
#if (CONFIG_LED == LED_VIRTUAL)
extern void statusbar_led(void);
#endif

#endif /* End HAVE_LCD_BITMAP */
#endif /* PLUGIN */
#endif /*  _ICONS_H_ */
