/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Ray Lambert
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
#ifndef _ABREPEAT_H_
#define _ABREPEAT_H_

#include "system.h"

#ifdef AB_REPEAT_ENABLE
#include "audio.h"
#include "kernel.h"
#include <stdbool.h>

#define AB_MARKER_NONE 0

#if (AB_REPEAT_ENABLE == 1)
#include "settings.h"
#endif

void ab_repeat_init(void);           
#if 0 /* Currently unused */
unsigned int ab_get_A_marker(void);
unsigned int ab_get_B_marker(void);
#endif /* if 0 */
bool ab_before_A_marker(unsigned int song_position);
bool ab_after_A_marker(unsigned int song_position);
void ab_jump_to_A_marker(void);
void ab_reset_markers(void);
void ab_set_A_marker(unsigned int song_position);
void ab_set_B_marker(unsigned int song_position);
#if (CONFIG_CODEC == SWCODEC)
void ab_end_of_track_report(void);
#endif
#ifdef HAVE_LCD_BITMAP
#include "screen_access.h"
void ab_draw_markers(struct screen * screen, int capacity,
                     int x0, int x1, int y, int h);
#endif

/* These functions really need to be inlined for speed */
extern unsigned int ab_A_marker;
extern unsigned int ab_B_marker;

static inline bool ab_A_marker_set(void)
{
    return ab_A_marker != AB_MARKER_NONE;
}

static inline bool ab_B_marker_set(void)
{
    return ab_B_marker != AB_MARKER_NONE;
}

static inline bool ab_repeat_mode_enabled(void)
{
#if (AB_REPEAT_ENABLE == 2)
    return ab_A_marker_set() || ab_B_marker_set();
#else
    return global_settings.repeat_mode == REPEAT_AB;
#endif
}

static inline bool ab_reached_B_marker(unsigned int song_position)
{
/* following is the size of the window in which we'll detect that the B marker
was hit; it must be larger than the frequency (in milliseconds) at which this 
function is called otherwise detection of the B marker will be unreliable */
#if (CONFIG_CODEC == SWCODEC)
/* On swcodec, the worst case seems to be 9600kHz with 1024 samples between
 * calls, meaning ~9 calls per second, look within 1/5 of a second */
#define B_MARKER_DETECT_WINDOW 200
#else
/* we assume that this function will be called on each system tick and derive
the window size from this with a generous margin of error (note: the number 
of ticks per second is given by HZ) */
#define B_MARKER_DETECT_WINDOW ((1000/HZ)*10)
#endif
    if (ab_B_marker != AB_MARKER_NONE)
    {
        if ( (song_position >= ab_B_marker) 
        && (song_position <= (ab_B_marker+B_MARKER_DETECT_WINDOW)) )
            return true;
    }
    return false;
}

#if (CONFIG_CODEC == SWCODEC)
static inline void ab_position_report(unsigned long position)
{
    if (ab_repeat_mode_enabled())
    {
        if ( !(audio_status() & AUDIO_STATUS_PAUSE) && 
                ab_reached_B_marker(position) )
        {
            ab_jump_to_A_marker();
        }
    }
}
#endif

#endif

#endif /* _ABREPEAT_H_ */
