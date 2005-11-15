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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef _ICONS_H_
#define _ICONS_H_

#include <lcd.h>

/* 
 * Icons of size 6x8 pixels 
 */

#ifdef HAVE_LCD_BITMAP

/* Symbolic names for icons */
enum icons_5x8 {
    Icon_Lock
};

enum icons_6x8 {
    Icon_Audio,
    Icon_Folder,
    Icon_Playlist,
    Icon_Cursor,
    Icon_Wps,
    Icon_Firmware,
    Icon_Font,
    Icon_Language,
    Icon_Config,
    Icon_Plugin,
    Icon_Bookmark,
    LastIcon
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
    Icon_Repeat,
    Icon_RepeatOne,
    Icon_Shuffle,
    Icon_DownArrow,
    Icon_UpArrow,
    Icon_RepeatAB,
    Icon_Last
};

extern const unsigned char bitmap_icons_5x8[1][5];
extern const unsigned char bitmap_icons_6x8[LastIcon][6];
extern const unsigned char bitmap_icons_7x8[Icon_Last][7];
#if CONFIG_LED == LED_VIRTUAL
extern const unsigned char bitmap_icon_disk[];
#endif

#if  LCD_WIDTH == 112 || LCD_WIDTH == 128 || (defined(HAVE_REMOTE_LCD) && LCD_REMOTE_WIDTH == 128)
extern const unsigned char rockbox112x37[];
#endif
#if defined(IRIVER_H300_SERIES) || (CONFIG_LCD == LCD_IPODCOLOR)
#define ROCKBOXLOGO_WIDTH 220
#define ROCKBOXLOGO_HEIGHT 68
extern const unsigned short rockboxlogo[];
#elif CONFIG_LCD == LCD_IPODNANO
#define ROCKBOXLOGO_WIDTH 176
#define ROCKBOXLOGO_HEIGHT 54
extern const unsigned short rockboxlogo[];
#elif LCD_WIDTH >= 160
extern const unsigned char rockbox160x53x2[];
#endif

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
#ifdef HAVE_RTC
extern void statusbar_time(int hour, int minute);
#endif
#if CONFIG_LED == LED_VIRTUAL
extern void	statusbar_led(void);
#endif

#endif /* End HAVE_LCD_BITMAP */

#endif /*  _ICONS_H_ */
