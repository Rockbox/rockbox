/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * user intereface of image viewers (jpeg, png, etc.)
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

#ifndef _IMGVIEW_IMGVIEW_H
#define _IMGVIEW_IMGVIEW_H

#include "plugin.h"

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define IMGVIEW_ZOOM_IN     BUTTON_PLAY
#define IMGVIEW_ZOOM_OUT    BUTTON_ON
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        BUTTON_F3
#define IMGVIEW_PREVIOUS    BUTTON_F2
#define IMGVIEW_MENU        BUTTON_OFF

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define IMGVIEW_ZOOM_IN     BUTTON_SELECT
#define IMGVIEW_ZOOM_OUT    BUTTON_ON
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        BUTTON_F3
#define IMGVIEW_PREVIOUS    BUTTON_F2
#define IMGVIEW_MENU        BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#define IMGVIEW_ZOOM_PRE    BUTTON_MENU
#define IMGVIEW_ZOOM_IN     (BUTTON_MENU | BUTTON_REL)
#define IMGVIEW_ZOOM_OUT    (BUTTON_MENU | BUTTON_DOWN)
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        (BUTTON_MENU | BUTTON_RIGHT)
#define IMGVIEW_PREVIOUS    (BUTTON_MENU | BUTTON_LEFT)
#define IMGVIEW_MENU        BUTTON_OFF

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define IMGVIEW_ZOOM_IN     BUTTON_SELECT
#define IMGVIEW_ZOOM_OUT    BUTTON_MODE
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#if (CONFIG_KEYPAD == IRIVER_H100_PAD)
#define IMGVIEW_NEXT        BUTTON_ON
#define IMGVIEW_PREVIOUS    BUTTON_REC
#else
#define IMGVIEW_NEXT        BUTTON_REC
#define IMGVIEW_PREVIOUS    BUTTON_ON
#endif
#define IMGVIEW_MENU        BUTTON_OFF
#define IMGVIEW_RC_MENU     BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define IMGVIEW_ZOOM_IN     BUTTON_SCROLL_FWD
#define IMGVIEW_ZOOM_OUT    BUTTON_SCROLL_BACK
#define IMGVIEW_UP          BUTTON_MENU
#define IMGVIEW_DOWN        BUTTON_PLAY
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        (BUTTON_SELECT | BUTTON_RIGHT)
#define IMGVIEW_PREVIOUS    (BUTTON_SELECT | BUTTON_LEFT)
#define IMGVIEW_MENU        (BUTTON_SELECT | BUTTON_MENU)

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define IMGVIEW_ZOOM_PRE    BUTTON_SELECT
#define IMGVIEW_ZOOM_IN     (BUTTON_SELECT | BUTTON_REL)
#define IMGVIEW_ZOOM_OUT    (BUTTON_SELECT | BUTTON_REPEAT)
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        BUTTON_PLAY
#define IMGVIEW_PREVIOUS    BUTTON_REC
#define IMGVIEW_MENU        BUTTON_POWER

#elif CONFIG_KEYPAD == GIGABEAT_PAD
#define IMGVIEW_ZOOM_IN     BUTTON_VOL_UP
#define IMGVIEW_ZOOM_OUT    BUTTON_VOL_DOWN
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        (BUTTON_A | BUTTON_RIGHT)
#define IMGVIEW_PREVIOUS    (BUTTON_A | BUTTON_LEFT)
#define IMGVIEW_MENU        BUTTON_MENU

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define IMGVIEW_ZOOM_PRE        BUTTON_SELECT
#define IMGVIEW_ZOOM_IN         (BUTTON_SELECT | BUTTON_REL)
#define IMGVIEW_ZOOM_OUT        (BUTTON_SELECT | BUTTON_REPEAT)
#define IMGVIEW_UP              BUTTON_UP
#define IMGVIEW_DOWN            BUTTON_DOWN
#define IMGVIEW_LEFT            BUTTON_LEFT
#define IMGVIEW_RIGHT           BUTTON_RIGHT
#define IMGVIEW_NEXT            BUTTON_SCROLL_FWD
#define IMGVIEW_NEXT_REPEAT     (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define IMGVIEW_PREVIOUS        BUTTON_SCROLL_BACK
#define IMGVIEW_PREVIOUS_REPEAT (BUTTON_SCROLL_BACK|BUTTON_REPEAT)
#define IMGVIEW_MENU            BUTTON_POWER
#define IMGVIEW_SLIDE_SHOW      BUTTON_REC

#elif CONFIG_KEYPAD == SANSA_FUZE_PAD
#define IMGVIEW_ZOOM_PRE        BUTTON_SELECT
#define IMGVIEW_ZOOM_IN         (BUTTON_SELECT | BUTTON_REL)
#define IMGVIEW_ZOOM_OUT        (BUTTON_SELECT | BUTTON_REPEAT)
#define IMGVIEW_UP              BUTTON_UP
#define IMGVIEW_DOWN            BUTTON_DOWN
#define IMGVIEW_LEFT            BUTTON_LEFT
#define IMGVIEW_RIGHT           BUTTON_RIGHT
#define IMGVIEW_NEXT            BUTTON_SCROLL_FWD
#define IMGVIEW_NEXT_REPEAT     (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define IMGVIEW_PREVIOUS        BUTTON_SCROLL_BACK
#define IMGVIEW_PREVIOUS_REPEAT (BUTTON_SCROLL_BACK|BUTTON_REPEAT)
#define IMGVIEW_MENU            (BUTTON_HOME|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_C200_PAD
#define IMGVIEW_ZOOM_PRE        BUTTON_SELECT
#define IMGVIEW_ZOOM_IN         (BUTTON_SELECT | BUTTON_REL)
#define IMGVIEW_ZOOM_OUT        (BUTTON_SELECT | BUTTON_REPEAT)
#define IMGVIEW_UP              BUTTON_UP
#define IMGVIEW_DOWN            BUTTON_DOWN
#define IMGVIEW_LEFT            BUTTON_LEFT
#define IMGVIEW_RIGHT           BUTTON_RIGHT
#define IMGVIEW_NEXT            BUTTON_VOL_UP
#define IMGVIEW_NEXT_REPEAT     (BUTTON_VOL_UP|BUTTON_REPEAT)
#define IMGVIEW_PREVIOUS        BUTTON_VOL_DOWN
#define IMGVIEW_PREVIOUS_REPEAT (BUTTON_VOL_DOWN|BUTTON_REPEAT)
#define IMGVIEW_MENU            BUTTON_POWER
#define IMGVIEW_SLIDE_SHOW      BUTTON_REC

#elif CONFIG_KEYPAD == SANSA_CLIP_PAD
#define IMGVIEW_ZOOM_PRE        BUTTON_SELECT
#define IMGVIEW_ZOOM_IN         (BUTTON_SELECT | BUTTON_REL)
#define IMGVIEW_ZOOM_OUT        (BUTTON_SELECT | BUTTON_REPEAT)
#define IMGVIEW_UP              BUTTON_UP
#define IMGVIEW_DOWN            BUTTON_DOWN
#define IMGVIEW_LEFT            BUTTON_LEFT
#define IMGVIEW_RIGHT           BUTTON_RIGHT
#define IMGVIEW_NEXT            BUTTON_VOL_UP
#define IMGVIEW_NEXT_REPEAT     (BUTTON_VOL_UP|BUTTON_REPEAT)
#define IMGVIEW_PREVIOUS        BUTTON_VOL_DOWN
#define IMGVIEW_PREVIOUS_REPEAT (BUTTON_VOL_DOWN|BUTTON_REPEAT)
#define IMGVIEW_MENU            BUTTON_POWER
#define IMGVIEW_SLIDE_SHOW      BUTTON_HOME

#elif CONFIG_KEYPAD == SANSA_M200_PAD
#define IMGVIEW_ZOOM_PRE        BUTTON_SELECT
#define IMGVIEW_ZOOM_IN         (BUTTON_SELECT | BUTTON_REL)
#define IMGVIEW_ZOOM_OUT        (BUTTON_SELECT | BUTTON_REPEAT)
#define IMGVIEW_UP              BUTTON_UP
#define IMGVIEW_DOWN            BUTTON_DOWN
#define IMGVIEW_LEFT            BUTTON_LEFT
#define IMGVIEW_RIGHT           BUTTON_RIGHT
#define IMGVIEW_NEXT            BUTTON_VOL_UP
#define IMGVIEW_NEXT_REPEAT     (BUTTON_VOL_UP|BUTTON_REPEAT)
#define IMGVIEW_PREVIOUS        BUTTON_VOL_DOWN
#define IMGVIEW_PREVIOUS_REPEAT (BUTTON_VOL_DOWN|BUTTON_REPEAT)
#define IMGVIEW_MENU            BUTTON_POWER
#define IMGVIEW_SLIDE_SHOW      (BUTTON_SELECT | BUTTON_UP)

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define IMGVIEW_ZOOM_PRE    BUTTON_PLAY
#define IMGVIEW_ZOOM_IN     (BUTTON_PLAY | BUTTON_REL)
#define IMGVIEW_ZOOM_OUT    (BUTTON_PLAY | BUTTON_REPEAT)
#define IMGVIEW_UP          BUTTON_SCROLL_UP
#define IMGVIEW_DOWN        BUTTON_SCROLL_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        BUTTON_FF
#define IMGVIEW_PREVIOUS    BUTTON_REW
#define IMGVIEW_MENU        BUTTON_POWER

#elif CONFIG_KEYPAD == MROBE500_PAD
#define IMGVIEW_MENU        BUTTON_POWER

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define IMGVIEW_ZOOM_IN     BUTTON_VOL_UP
#define IMGVIEW_ZOOM_OUT    BUTTON_VOL_DOWN
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        BUTTON_NEXT
#define IMGVIEW_PREVIOUS    BUTTON_PREV
#define IMGVIEW_MENU        BUTTON_MENU

#elif CONFIG_KEYPAD == MROBE100_PAD
#define IMGVIEW_ZOOM_IN     BUTTON_SELECT
#define IMGVIEW_ZOOM_OUT    BUTTON_PLAY
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        (BUTTON_DISPLAY | BUTTON_RIGHT)
#define IMGVIEW_PREVIOUS    (BUTTON_DISPLAY | BUTTON_LEFT)
#define IMGVIEW_MENU        BUTTON_MENU

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD
#define IMGVIEW_ZOOM_PRE    BUTTON_RC_PLAY
#define IMGVIEW_ZOOM_IN     (BUTTON_RC_PLAY|BUTTON_REL)
#define IMGVIEW_ZOOM_OUT    (BUTTON_RC_PLAY|BUTTON_REPEAT)
#define IMGVIEW_UP          BUTTON_RC_VOL_UP
#define IMGVIEW_DOWN        BUTTON_RC_VOL_DOWN
#define IMGVIEW_LEFT        BUTTON_RC_REW
#define IMGVIEW_RIGHT       BUTTON_RC_FF
#define IMGVIEW_NEXT        BUTTON_RC_MODE
#define IMGVIEW_PREVIOUS    BUTTON_RC_MENU
#define IMGVIEW_MENU        BUTTON_RC_REC

#elif CONFIG_KEYPAD == COWON_D2_PAD

#elif CONFIG_KEYPAD == IAUDIO67_PAD
#define IMGVIEW_ZOOM_IN     BUTTON_VOLUP
#define IMGVIEW_ZOOM_OUT    BUTTON_VOLDOWN
#define IMGVIEW_UP          BUTTON_STOP
#define IMGVIEW_DOWN        BUTTON_PLAY
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        (BUTTON_PLAY|BUTTON_VOLUP)
#define IMGVIEW_PREVIOUS    (BUTTON_PLAY|BUTTON_VOLDOWN)
#define IMGVIEW_MENU        BUTTON_MENU

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD

#define IMGVIEW_ZOOM_IN     BUTTON_PLAY
#define IMGVIEW_ZOOM_OUT    BUTTON_CUSTOM
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        BUTTON_SELECT
#define IMGVIEW_PREVIOUS    BUTTON_BACK
#define IMGVIEW_MENU        BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define IMGVIEW_ZOOM_IN     BUTTON_VOL_UP
#define IMGVIEW_ZOOM_OUT    BUTTON_VOL_DOWN
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        BUTTON_VIEW
#define IMGVIEW_PREVIOUS    BUTTON_PLAYLIST
#define IMGVIEW_MENU        BUTTON_MENU

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define IMGVIEW_ZOOM_IN     BUTTON_VOL_UP
#define IMGVIEW_ZOOM_OUT    BUTTON_VOL_DOWN
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_PREV
#define IMGVIEW_RIGHT       BUTTON_NEXT
#define IMGVIEW_NEXT        BUTTON_RIGHT
#define IMGVIEW_PREVIOUS    BUTTON_LEFT
#define IMGVIEW_MENU        BUTTON_MENU

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#elif CONFIG_KEYPAD == ONDAVX777_PAD

#elif CONFIG_KEYPAD == SAMSUNG_YH_PAD
#define IMGVIEW_ZOOM_IN     (BUTTON_PLAY|BUTTON_UP)
#define IMGVIEW_ZOOM_OUT    (BUTTON_PLAY|BUTTON_DOWN)
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_LEFT
#define IMGVIEW_RIGHT       BUTTON_RIGHT
#define IMGVIEW_NEXT        BUTTON_FFWD
#define IMGVIEW_PREVIOUS    BUTTON_REW
#define IMGVIEW_MENU        BUTTON_PLAY

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define IMGVIEW_ZOOM_IN     (BUTTON_REC | BUTTON_UP)
#define IMGVIEW_ZOOM_OUT    (BUTTON_REC | BUTTON_DOWN)
#define IMGVIEW_UP          BUTTON_UP
#define IMGVIEW_DOWN        BUTTON_DOWN
#define IMGVIEW_LEFT        BUTTON_PREV
#define IMGVIEW_RIGHT       BUTTON_NEXT
#define IMGVIEW_NEXT        (BUTTON_REC | BUTTON_NEXT)
#define IMGVIEW_PREVIOUS    (BUTTON_REC | BUTTON_PREV)
#define IMGVIEW_MENU        BUTTON_MENU

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef IMGVIEW_UP
#define IMGVIEW_UP          BUTTON_TOPMIDDLE
#endif
#ifndef IMGVIEW_DOWN
#define IMGVIEW_DOWN        BUTTON_BOTTOMMIDDLE
#endif
#ifndef IMGVIEW_LEFT
#define IMGVIEW_LEFT        BUTTON_MIDLEFT
#endif
#ifndef IMGVIEW_RIGHT
#define IMGVIEW_RIGHT       BUTTON_MIDRIGHT
#endif
#ifndef IMGVIEW_ZOOM_IN
#define IMGVIEW_ZOOM_IN     BUTTON_TOPRIGHT
#endif
#ifndef IMGVIEW_ZOOM_OUT
#define IMGVIEW_ZOOM_OUT    BUTTON_TOPLEFT
#endif
#ifndef IMGVIEW_MENU
#define IMGVIEW_MENU        (BUTTON_CENTER|BUTTON_REL)
#endif
#ifndef IMGVIEW_NEXT
#define IMGVIEW_NEXT        BUTTON_BOTTOMRIGHT
#endif
#ifndef IMGVIEW_PREVIOUS
#define IMGVIEW_PREVIOUS    BUTTON_BOTTOMLEFT
#endif
#endif

/* different graphics libraries */
#if LCD_DEPTH < 8
#define USEGSLIB
#include <lib/grey.h>
#define MYLCD(fn) grey_ub_ ## fn
#define MYLCD_UPDATE()
#define MYXLCD(fn) grey_ub_ ## fn
#else
#include <lib/xlcd.h>
#define MYLCD(fn) rb->lcd_ ## fn
#define MYLCD_UPDATE() rb->lcd_update();
#define MYXLCD(fn) xlcd_ ## fn
#endif

/* Min memory allowing us to use the plugin buffer
 * and thus not stopping the music
 * *Very* rough estimation:
 * Max 10 000 dir entries * 4bytes/entry (char **) = 40000 bytes
 * + 30k code size = 70 000
 * + 50k min for image = 120 000
 */
#define MIN_MEM 120000

/* State code for output with return. */
#define PLUGIN_OTHER    10
#define PLUGIN_ABORT    11
#define PLUGIN_OUTOFMEM 12

#if !defined(SIMULATOR) && defined(HAVE_DISK_STORAGE)
#define DISK_SPINDOWN
#endif
#if PLUGIN_BUFFER_SIZE >= MIN_MEM
#define USE_PLUG_BUF
#endif

/* Settings. jpeg needs these */
struct imgview_settings
{
    /* include all settings for varias decoders as using same setting file. */
#ifdef HAVE_LCD_COLOR
    int jpeg_colour_mode;
    int jpeg_dither_mode;
#endif
    int ss_timeout;
};

/* structure passed to decoder. */
struct image_info {
    int x_size, y_size; /* set size of loaded image in load_image(). */
    int width, height;  /* set size of resized image in get_image(). */
    int x, y;           /* display position */
    void *data;         /* use freely in decoder. not touched in ui. */
};

/* callback updating a progress meter while image decoding */
extern void cb_progress(int current, int total);

extern struct imgview_settings settings;
extern bool slideshow_enabled;
extern bool running_slideshow;
#ifdef DISK_SPINDOWN
extern bool immediate_ata_off;
#endif
#ifdef USE_PLUG_BUF
extern bool plug_buf;
#endif

/* functions need to be implemented in each image decoders. */
/* return true if ext is supported by the decoder. */
extern bool img_ext(const char *ext);
/* return needed size of buffer to store downscaled image by ds */
extern int img_mem(int ds);
/* load image from filename. set width and height of info properly. also, set
 * buf_size to remaining size of buf after load image. it is used to caluclate
 * min downscale. */
extern int load_image(char *filename, struct image_info *info,
                      unsigned char *buf, ssize_t *buf_size);
/* downscale loaded image by ds. note that buf to store reszied image is not
 * provided. return PLUGIN_ERROR for error. ui will skip to next image. */
extern int get_image(struct image_info *info, int ds);
/* draw part of image */
extern void draw_image_rect(struct image_info *info,
                            int x, int y, int width, int height);

#endif /* _IMGVIEW_IMGVIEW_H */
