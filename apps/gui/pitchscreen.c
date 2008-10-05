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
#include "statusbar.h"
#include "viewport.h"
#include "pitchscreen.h"

#define PITCH_MODE_ABSOLUTE 1
#define PITCH_MODE_SEMITONE -PITCH_MODE_ABSOLUTE
#define ICON_BORDER 12

static int pitch_mode = PITCH_MODE_ABSOLUTE; /* 1 - absolute, -1 - semitone */
enum PITCHSCREEN_VALUES
{
    PITCH_SMALL_DELTA =     1,
    PITCH_BIG_DELTA   =    10,
    PITCH_NUDGE_DELTA =    20,
    PITCH_MIN         =   500,
    PITCH_MAX         =  2000,
};

enum PITCHSCREEN_ITEMS
{
    PITCH_TOP = 0,
    PITCH_MID,
    PITCH_BOTTOM,
    PITCH_ITEM_COUNT,
};

static void pitchscreen_fix_viewports(enum screen_type screen,
        struct viewport *parent,
        struct viewport pitch_viewports[NB_SCREENS][PITCH_ITEM_COUNT])
{
    short n, height;
    height = font_get(parent->font)->height;
    for (n = 0; n < PITCH_ITEM_COUNT; n++)
    {
        pitch_viewports[screen][n] = *parent;
        pitch_viewports[screen][n].height = height;
    }
    pitch_viewports[screen][PITCH_TOP].y += ICON_BORDER;

    pitch_viewports[screen][PITCH_MID].x += ICON_BORDER;
    pitch_viewports[screen][PITCH_MID].width = parent->width - ICON_BORDER*2;
    pitch_viewports[screen][PITCH_MID].height = height * 2;
    pitch_viewports[screen][PITCH_MID].y += parent->height / 2 -
            pitch_viewports[screen][PITCH_MID].height / 2;
    pitch_viewports[screen][PITCH_BOTTOM].y += parent->height - height -
            ICON_BORDER;
}

/* must be called before pitchscreen_draw, or within
 * since it neither clears nor updates the display */
static void pitchscreen_draw_icons (struct screen *display,
        struct viewport *parent)
{
    display->set_viewport(parent);
    display->mono_bitmap(bitmap_icons_7x8[Icon_UpArrow],
            parent->width/2 - 3,
            2, 7, 8);
    display->mono_bitmap(bitmap_icons_7x8[Icon_DownArrow],
            parent->width /2 - 3,
            parent->height - 10, 7, 8);
    display->mono_bitmap(bitmap_icons_7x8[Icon_FastForward],
            parent->width - 10,
            parent->height /2 - 4, 7, 8);
    display->mono_bitmap(bitmap_icons_7x8[Icon_FastBackward],
            2,
            parent->height /2 - 4, 7, 8);
}

static void pitchscreen_draw (
        struct screen *display,
        int max_lines,
        struct viewport pitch_viewports[PITCH_ITEM_COUNT],
        int pitch)
{
    unsigned char* ptr;
    unsigned char buf[32];
    int w, h;
    bool show_lang_pitch;

     /* Hide "Pitch up/Pitch down" for a small screen */
    if (max_lines >= 5)
    {
        /* UP: Pitch Up */
        display->set_viewport(&pitch_viewports[PITCH_TOP]);
        if (pitch_mode == PITCH_MODE_ABSOLUTE) {
            ptr = str(LANG_PITCH_UP);
        } else {
            ptr = str(LANG_PITCH_UP_SEMITONE);
        }
        display->getstringsize(ptr,&w,&h);
        display->clear_viewport();
        /* draw text */
        display->putsxy((pitch_viewports[PITCH_TOP].width / 2) -
                (w / 2), 0, ptr);

        /* DOWN: Pitch Down */
        display->set_viewport(&pitch_viewports[PITCH_BOTTOM]);
        if (pitch_mode == PITCH_MODE_ABSOLUTE) {
            ptr = str(LANG_PITCH_DOWN);
        } else {
            ptr = str(LANG_PITCH_DOWN_SEMITONE);
        }
        display->getstringsize(ptr,&w,&h);
        display->clear_viewport();
        /* draw text */
        display->putsxy((pitch_viewports[PITCH_BOTTOM].width / 2) -
                (w / 2), 0, ptr);
    }
    display->set_viewport(&pitch_viewports[PITCH_MID]);

    snprintf((char *)buf, sizeof(buf), "%s", str(LANG_PITCH));
    display->getstringsize(buf,&w,&h);
    /* lets hide LANG_PITCH for smaller screens */
    display->clear_viewport();
    if ((show_lang_pitch = (max_lines >= 3)))
        display->putsxy((pitch_viewports[PITCH_MID].width / 2) - (w / 2),
                0, buf);

    /* we don't need max_lines any more, reuse it*/
    max_lines = w;
    /* "XXX.X%" */
    snprintf((char *)buf, sizeof(buf), "%d.%d%%",
            pitch / 10, pitch % 10 );
    display->getstringsize(buf,&w,&h);
    display->putsxy((pitch_viewports[PITCH_MID].width / 2) - (w / 2),
            (show_lang_pitch? h : h/2), buf);

    /* What's wider? LANG_PITCH or the value?
     * Only interesting if LANG_PITCH is actually drawn */
    max_lines = (show_lang_pitch ? ((max_lines > w) ? max_lines : w) : w);

    /* Let's treat '+' and '-' as equally wide
     * This saves a getstringsize call
     * Also, it wouldn't look nice if -2% shows up, but +2% not */
    display->getstringsize("+2%",&w,&h);
    max_lines += 2*w;
    /* hide +2%/-2% for a narrow screens */
    if (max_lines < pitch_viewports[PITCH_MID].width)
    {
        /* RIGHT: +2% */
        display->putsxy(pitch_viewports[PITCH_MID].width - w, h /2, "+2%");
        /* LEFT: -2% */
        display->putsxy(0, h / 2, "-2%");
    }
    /* Lastly, a fullscreen update */
    display->set_viewport(NULL);
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

/*
    returns:
    0 on exit
    1 if USB was connected
*/

int gui_syncpitchscreen_run(void)
{
    int button;
    int pitch = sound_get_pitch();
    int new_pitch, delta = 0;
    bool nudged = false;
    bool exit = false;
    short i;
    struct viewport parent[NB_SCREENS]; /* should be a parameter of this function */
    short max_lines[NB_SCREENS];
    struct viewport pitch_viewports[NB_SCREENS][PITCH_ITEM_COUNT];

    /* initialize pitchscreen vps */
    FOR_NB_SCREENS(i)
    {
        screens[i].clear_display();
        viewport_set_defaults(&parent[i], i);
        max_lines[i] = viewport_get_nb_lines(&parent[i]);
        pitchscreen_fix_viewports(i, &parent[i], pitch_viewports);

        /* also, draw the icons now, it's only needed once */
        pitchscreen_draw_icons(&screens[i], &parent[i]);
    }
#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(true);
#endif
    i = 0;
    while (!exit)
    {
        FOR_NB_SCREENS(i)
            pitchscreen_draw(&screens[i], max_lines[i],
                              pitch_viewports[i], pitch);
        gui_syncstatusbar_draw(&statusbars, true);
        button = get_action(CONTEXT_PITCHSCREEN,HZ);
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
    return 0;
}
