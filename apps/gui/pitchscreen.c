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
#include "action.h"
#include "dsp.h"
#include "sound.h"
#include "pcmbuf.h"
#include "lang.h"
#include "icons.h"
#include "screens.h"
#include "viewport.h"
#include "font.h"
#include "system.h"
#include "misc.h"
#include "pitchscreen.h"
#if CONFIG_CODEC == SWCODEC
#include "tdspeed.h"
#endif


#define ICON_BORDER 12 /* icons are currently 7x8, so add ~2 pixels  */
                       /*   on both sides when drawing */

#define PITCH_MAX         2000
#define PITCH_MIN         500
#define PITCH_SMALL_DELTA 1
#define PITCH_BIG_DELTA   10
#define PITCH_NUDGE_DELTA 20

static bool pitch_mode_semitone = false;
#if CONFIG_CODEC == SWCODEC
static bool pitch_mode_timestretch = false;
#endif

enum
{
    PITCH_TOP = 0,
    PITCH_MID,
    PITCH_BOTTOM,
    PITCH_ITEM_COUNT,
};

static void pitchscreen_fix_viewports(struct viewport *parent,
        struct viewport pitch_viewports[PITCH_ITEM_COUNT])
{
    int i, height;
    height = font_get(parent->font)->height;
    for (i = 0; i < PITCH_ITEM_COUNT; i++)
    {
        pitch_viewports[i] = *parent;
        pitch_viewports[i].height = height;
    }
    pitch_viewports[PITCH_TOP].y += ICON_BORDER;

    pitch_viewports[PITCH_MID].x += ICON_BORDER;
    pitch_viewports[PITCH_MID].width = parent->width - ICON_BORDER*2;
    pitch_viewports[PITCH_MID].height = height * 2;
    pitch_viewports[PITCH_MID].y += parent->height / 2 -
            pitch_viewports[PITCH_MID].height / 2;

    pitch_viewports[PITCH_BOTTOM].y += parent->height - height - ICON_BORDER;
}

/* must be called before pitchscreen_draw, or within
 * since it neither clears nor updates the display */
static void pitchscreen_draw_icons(struct screen *display,
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
    display->update_viewport();
}

static void pitchscreen_draw(struct screen *display, int max_lines,
                             struct viewport pitch_viewports[PITCH_ITEM_COUNT],
                             int pitch
#if CONFIG_CODEC == SWCODEC
                             ,int speed
#endif
                             )
{
    unsigned char* ptr;
    char buf[32];
    int w, h;
    bool show_lang_pitch;

     /* "Pitch up/Pitch down" - hide for a small screen */
    if (max_lines >= 5)
    {
        /* UP: Pitch Up */
        display->set_viewport(&pitch_viewports[PITCH_TOP]);
        if (pitch_mode_semitone)
            ptr = str(LANG_PITCH_UP_SEMITONE);
        else
            ptr = str(LANG_PITCH_UP);
        display->getstringsize(ptr, &w, &h);
        display->clear_viewport();
        /* draw text */
        display->putsxy((pitch_viewports[PITCH_TOP].width / 2) -
                (w / 2), 0, ptr);
        display->update_viewport();

        /* DOWN: Pitch Down */
        display->set_viewport(&pitch_viewports[PITCH_BOTTOM]);
        if (pitch_mode_semitone)
            ptr = str(LANG_PITCH_DOWN_SEMITONE);
        else
            ptr = str(LANG_PITCH_DOWN);
        display->getstringsize(ptr, &w, &h);
        display->clear_viewport();
        /* draw text */
        display->putsxy((pitch_viewports[PITCH_BOTTOM].width / 2) -
                (w / 2), 0, ptr);
        display->update_viewport();
    }

    /* Middle section */
    display->set_viewport(&pitch_viewports[PITCH_MID]);
    display->clear_viewport();
    int width_used = 0;

    /* Middle section upper line - hide for a small screen */
    if ((show_lang_pitch = (max_lines >= 3)))
    {
#if CONFIG_CODEC == SWCODEC
        if (!pitch_mode_timestretch)
        {
#endif
            /* LANG_PITCH */
            snprintf(buf, sizeof(buf), "%s", str(LANG_PITCH));
#if CONFIG_CODEC == SWCODEC
        }
        else
        {
            /* Pitch:XXX.X% */
            snprintf(buf, sizeof(buf), "%s:%d.%d%%", str(LANG_PITCH),
                     pitch / 10, pitch % 10);
        }
#endif
        display->getstringsize(buf, &w, &h);
        display->putsxy((pitch_viewports[PITCH_MID].width / 2) - (w / 2),
                        0, buf);
        if (w > width_used)
            width_used = w;
    }

    /* Middle section lower line */
#if CONFIG_CODEC == SWCODEC
    if (!pitch_mode_timestretch)
    {
#endif
        /* "XXX.X%" */
        snprintf(buf, sizeof(buf), "%d.%d%%",
             pitch / 10, pitch % 10);
#if CONFIG_CODEC == SWCODEC
    }
    else
    {
        /* "Speed:XXX%" */
        snprintf(buf, sizeof(buf), "%s:%d%%", str(LANG_SPEED), 
             speed / 1000);
    }
#endif
    display->getstringsize(buf, &w, &h);
    display->putsxy((pitch_viewports[PITCH_MID].width / 2) - (w / 2),
        (show_lang_pitch ? h : h/2), buf);
    if (w > width_used)
        width_used = w;

    /* Middle section left/right labels */
    const char *leftlabel = "-2%";
    const char *rightlabel = "+2%";
#if CONFIG_CODEC == SWCODEC
    if (pitch_mode_timestretch)
    {
        leftlabel = "<<";
        rightlabel = ">>";
    }
#endif

    /* Only display if they fit */
    display->getstringsize(leftlabel, &w, &h);
    width_used += w;
    display->getstringsize(rightlabel, &w, &h);
    width_used += w;

    if (width_used <= pitch_viewports[PITCH_MID].width)
    {
        display->putsxy(0, h / 2, leftlabel);
        display->putsxy(pitch_viewports[PITCH_MID].width - w, h /2, rightlabel);
    }
    display->update_viewport();
    display->set_viewport(NULL);
}

static int pitch_increase(int pitch, int pitch_delta, bool allow_cutoff)
{
    int new_pitch;

    if (pitch_delta < 0)
    {
        if (pitch + pitch_delta >= PITCH_MIN)
            new_pitch = pitch + pitch_delta;
        else
        {
            if (!allow_cutoff)
                return pitch;
            new_pitch = PITCH_MIN;
        }
    }
    else if (pitch_delta > 0)
    {
        if (pitch + pitch_delta <= PITCH_MAX)
            new_pitch = pitch + pitch_delta;
        else
        {
            if (!allow_cutoff)
                return pitch;
            new_pitch = PITCH_MAX;
        }
    }
    else
    {
        /* pitch_delta == 0 -> no real change */
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
    if (up)
    {
        tmp = tmp * PITCH_SEMITONE_FACTOR;
        round_fct = PITCH_K_FCT;
    }
    else
    {
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
    int button, i;
    int pitch = sound_get_pitch();
#if CONFIG_CODEC == SWCODEC
    int stretch = dsp_get_timestretch();
    int speed = stretch * pitch; /* speed to maintain */
#endif
    int new_pitch;
    int pitch_delta;
    bool nudged = false;
    bool exit = false;
    /* should maybe be passed per parameter later, not needed for now */
    struct viewport parent[NB_SCREENS];
    struct viewport pitch_viewports[NB_SCREENS][PITCH_ITEM_COUNT];
    int max_lines[NB_SCREENS];

    /* initialize pitchscreen vps */
    FOR_NB_SCREENS(i)
    {
        screens[i].clear_display();
        viewport_set_defaults(&parent[i], i);
        max_lines[i] = viewport_get_nb_lines(&parent[i]);
        pitchscreen_fix_viewports(&parent[i], pitch_viewports[i]);

        /* also, draw the icons now, it's only needed once */
        pitchscreen_draw_icons(&screens[i], &parent[i]);
    }
#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(true);
#endif

    while (!exit)
    {
        FOR_NB_SCREENS(i)
            pitchscreen_draw(&screens[i], max_lines[i],
                              pitch_viewports[i], pitch
#if CONFIG_CODEC == SWCODEC
                              , speed
#endif
                              );
        pitch_delta = 0;
        button = get_action(CONTEXT_PITCHSCREEN, HZ);
        switch (button)
        {
            case ACTION_PS_INC_SMALL:
                pitch_delta = PITCH_SMALL_DELTA;
                break;

            case ACTION_PS_INC_BIG:
                pitch_delta = PITCH_BIG_DELTA;
                break;

            case ACTION_PS_DEC_SMALL:
                pitch_delta = -PITCH_SMALL_DELTA;
                break;

            case ACTION_PS_DEC_BIG:
                pitch_delta = -PITCH_BIG_DELTA;
                break;

            case ACTION_PS_NUDGE_RIGHT:
#if CONFIG_CODEC == SWCODEC
                if (!pitch_mode_timestretch)
                {
#endif
                    new_pitch = pitch_increase(pitch, PITCH_NUDGE_DELTA, false);
                    nudged = (new_pitch != pitch);
                    pitch = new_pitch;
                    break;
#if CONFIG_CODEC == SWCODEC
                }

            case ACTION_PS_FASTER:
                if (pitch_mode_timestretch && stretch < STRETCH_MAX)
                {
                    stretch++;
                    dsp_set_timestretch(stretch);
                    speed = stretch * pitch;
                }
                break;
#endif

            case ACTION_PS_NUDGE_RIGHTOFF:
                if (nudged)
                {
                    pitch = pitch_increase(pitch, -PITCH_NUDGE_DELTA, false);
                    nudged = false;
                }
                break;

            case ACTION_PS_NUDGE_LEFT:
#if CONFIG_CODEC == SWCODEC
                if (!pitch_mode_timestretch)
                {
#endif
                    new_pitch = pitch_increase(pitch, -PITCH_NUDGE_DELTA, false);
                    nudged = (new_pitch != pitch);
                    pitch = new_pitch;
                    break;
#if CONFIG_CODEC == SWCODEC
                }

            case ACTION_PS_SLOWER:
                if (pitch_mode_timestretch && stretch > STRETCH_MIN)
                {
                    stretch--;
                    dsp_set_timestretch(stretch);
                    speed = stretch * pitch;
                }
                break;
#endif

            case ACTION_PS_NUDGE_LEFTOFF:
                if (nudged)
                {
                    pitch = pitch_increase(pitch, PITCH_NUDGE_DELTA, false);
                    nudged = false;
                }
                break;

            case ACTION_PS_RESET:
                pitch = 1000;
                sound_set_pitch(pitch);
#if CONFIG_CODEC == SWCODEC
                stretch = 100;
                dsp_set_timestretch(stretch);
                speed = stretch * pitch;
#endif
                break;

            case ACTION_PS_TOGGLE_MODE:
#if CONFIG_CODEC == SWCODEC
                if (dsp_timestretch_available() && pitch_mode_semitone)
                    pitch_mode_timestretch = !pitch_mode_timestretch;
#endif
                pitch_mode_semitone = !pitch_mode_semitone;
                break;

            case ACTION_PS_EXIT:
                exit = true;
                break;

            default:
                if (default_event_handler(button) == SYS_USB_CONNECTED)
                    return 1;
                break;
        }
        if (pitch_delta)
        {
            if (pitch_mode_semitone)
                pitch = pitch_increase_semitone(pitch, pitch_delta > 0);
            else
                pitch = pitch_increase(pitch, pitch_delta, true);
#if CONFIG_CODEC == SWCODEC
            if (pitch_mode_timestretch)
            {
                /* Set stretch to maintain speed  */
                /* i.e. increase pitch, reduce stretch */
                int new_stretch = speed / pitch;
                if (new_stretch >= STRETCH_MIN && new_stretch <= STRETCH_MAX)
                {
                    stretch = new_stretch;
                    dsp_set_timestretch(stretch);
                }
            }
            else
                speed = stretch * new_pitch;
#endif           
        }
    }
#if CONFIG_CODEC == SWCODEC
    pcmbuf_set_low_latency(false);
#endif
    return 0;
}
