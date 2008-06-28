/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
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

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include "sprintf.h"
#include "settings.h"
#include "action.h"
#include "system.h"
#include "font.h"
#include "misc.h"
#include "dsp.h"
#include "sound.h"
#include "pcmbuf.h"
#include "lang.h"
#include "icons.h"
#include "screen_access.h"
#include "screens.h"

#define PITCH_MAX         2000
#define PITCH_MIN         500
#define PITCH_SMALL_DELTA 1
#define PITCH_BIG_DELTA   10
#define PITCH_NUDGE_DELTA 20

#define PITCH_MODE_ABSOLUTE 1
#define PITCH_MODE_SEMITONE -PITCH_MODE_ABSOLUTE

static int pitch_mode = PITCH_MODE_ABSOLUTE; /* 1 - absolute, -1 - semitone */

/* returns:
   0 if no key was pressed
   1 if USB was connected */

static void pitch_screen_draw(struct screen *display, int pitch, int pitch_mode)
{
    unsigned char* ptr;
    unsigned char buf[32];
    int w, h;

    display->clear_display();

    if (display->nb_lines < 4) /* very small screen, just show the pitch value */
    {
        w = snprintf((char *)buf, sizeof(buf), "%s: %d.%d%%",str(LANG_PITCH),
                  pitch / 10, pitch % 10 );
        display->putsxy((display->lcdwidth-(w*display->char_width))/2,
                         display->nb_lines/2,buf);
    }
    else /* bigger screen, show everything... */
    {

        /* UP: Pitch Up */
        if (pitch_mode == PITCH_MODE_ABSOLUTE) {
            ptr = str(LANG_PITCH_UP);
        } else {
            ptr = str(LANG_PITCH_UP_SEMITONE);
        }
        display->getstringsize(ptr,&w,&h);
        display->putsxy((display->lcdwidth-w)/2, 0, ptr);
        display->mono_bitmap(bitmap_icons_7x8[Icon_UpArrow],
                        display->lcdwidth/2 - 3, h, 7, 8);

        /* DOWN: Pitch Down */
        if (pitch_mode == PITCH_MODE_ABSOLUTE) {
            ptr = str(LANG_PITCH_DOWN);
        } else {
            ptr = str(LANG_PITCH_DOWN_SEMITONE);
        }
        display->getstringsize(ptr,&w,&h);
        display->putsxy((display->lcdwidth-w)/2, display->lcdheight - h, ptr);
        display->mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
                             display->lcdwidth/2 - 3,
                             display->lcdheight - h*2, 7, 8);

        /* RIGHT: +2% */
        ptr = "+2%";
        display->getstringsize(ptr,&w,&h);
        display->putsxy(display->lcdwidth-w, (display->lcdheight-h)/2, ptr);
        display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
                             display->lcdwidth-w-8,
                             (display->lcdheight-h)/2, 7, 8);

        /* LEFT: -2% */
        ptr = "-2%";
        display->getstringsize(ptr,&w,&h);
        display->putsxy(0, (display->lcdheight-h)/2, ptr);
        display->mono_bitmap(bitmap_icons_7x8[Icon_FastBackward],
                             w+1, (display->lcdheight-h)/2, 7, 8);

        /* "Pitch" */
        snprintf((char *)buf, sizeof(buf), str(LANG_PITCH));
        display->getstringsize(buf,&w,&h);
        display->putsxy((display->lcdwidth-w)/2, (display->lcdheight/2)-h, buf);
        /* "XX.X%" */
        snprintf((char *)buf, sizeof(buf), "%d.%d%%",
                pitch / 10, pitch % 10 );
        display->getstringsize(buf,&w,&h);
        display->putsxy((display->lcdwidth-w)/2, display->lcdheight/2, buf);
    }

    display->update();
}

static int pitch_increase(int pitch, int delta, bool allow_cutoff)
{
    int new_pitch;

    if (delta < 0) {
        if (pitch + delta >= PITCH_MIN) {
            new_pitch = pitch + delta;
        } else {
            if (!allow_cutoff) {
                return pitch;
            }
            new_pitch = PITCH_MIN;
        }
    } else if (delta > 0) {
        if (pitch + delta <= PITCH_MAX) {
            new_pitch = pitch + delta;
        } else {
            if (!allow_cutoff) {
                return pitch;
            }
            new_pitch = PITCH_MAX;
        }
    } else {
        /* delta == 0 -> no real change */
        return pitch;
    }
    sound_set_pitch(new_pitch);

    return new_pitch;
}

/* Factor for changing the pitch one half tone up.
   The exact value is 2^(1/12) = 1.05946309436
   But we use only integer arithmetics, so take
   rounded factor multiplied by 10^5=100,000. This is
   enough to get the same promille values as if we
   had used floating point (checked with a spread
   sheet).
 */
#define PITCH_SEMITONE_FACTOR 105946L

/* Some helpful constants. K is the scaling factor for SEMITONE.
   N is for more accurate rounding
   KN is K * N
 */
#define PITCH_K_FCT           100000UL
#define PITCH_N_FCT           10
#define PITCH_KN_FCT          1000000UL

static int pitch_increase_semitone(int pitch, bool up)
{
    uint32_t tmp;
    uint32_t round_fct; /* How much to scale down at the end */
    tmp = pitch;
    if (up) {
        tmp = tmp * PITCH_SEMITONE_FACTOR;
        round_fct = PITCH_K_FCT;
    } else {
        tmp = (tmp * PITCH_KN_FCT) / PITCH_SEMITONE_FACTOR;
        round_fct = PITCH_N_FCT;
    }
    /* Scaling down with rounding */
    tmp = (tmp + round_fct / 2) / round_fct;
    return pitch_increase(pitch, tmp - pitch, false);
}

bool pitch_screen(void)
{
    int button;
    int pitch = sound_get_pitch();
    int new_pitch, delta = 0;
    bool nudged = false;
    bool exit = false;
    int i;

#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(true);
#endif

    while (!exit)
    {
        FOR_NB_SCREENS(i)
            pitch_screen_draw(&screens[i], pitch, pitch_mode);

        button = get_action(CONTEXT_PITCHSCREEN,TIMEOUT_BLOCK);
        switch (button) {
            case ACTION_PS_INC_SMALL:
                delta = PITCH_SMALL_DELTA;
                break;

            case ACTION_PS_INC_BIG:
                delta = PITCH_BIG_DELTA;
                break;

            case ACTION_PS_DEC_SMALL:
                delta = -PITCH_SMALL_DELTA;
                break;

            case ACTION_PS_DEC_BIG:
                delta = -PITCH_BIG_DELTA;
                break;

            case ACTION_PS_NUDGE_RIGHT:
                new_pitch = pitch_increase(pitch, PITCH_NUDGE_DELTA, false);
                nudged = (new_pitch != pitch);
                pitch = new_pitch;
                break;

            case ACTION_PS_NUDGE_RIGHTOFF:
                if (nudged) {
                    pitch = pitch_increase(pitch, -PITCH_NUDGE_DELTA, false);
                }
                nudged = false;
                break;

            case ACTION_PS_NUDGE_LEFT:
                new_pitch = pitch_increase(pitch, -PITCH_NUDGE_DELTA, false);
                nudged = (new_pitch != pitch);
                pitch = new_pitch;
                break;
                
            case ACTION_PS_NUDGE_LEFTOFF:
                if (nudged) {
                    pitch = pitch_increase(pitch, PITCH_NUDGE_DELTA, false);
                }
                nudged = false;
                break;

            case ACTION_PS_RESET:
                pitch = 1000;
                sound_set_pitch( pitch );
                break;

            case ACTION_PS_TOGGLE_MODE:
                pitch_mode = -pitch_mode;
                break;

            case ACTION_PS_EXIT:
                exit = true;
                break;

            default:
                if(default_event_handler(button) == SYS_USB_CONNECTED)
                    return 1;
                break;
        }
        
        if(delta)
        {
            if (pitch_mode == PITCH_MODE_ABSOLUTE) {
                pitch = pitch_increase(pitch, delta, true);
            } else {
                pitch = pitch_increase_semitone(pitch, delta > 0 ? true:false);
            }
            
            delta = 0;
        }
        
    }
#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(false);
#endif
    lcd_setfont(FONT_UI);
    return 0;
}
