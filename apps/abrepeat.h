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

#ifdef AB_REPEAT_ENABLE
#include <stdbool.h>
#include "audio.h"
#include "kernel.h" /* needed for HZ */

#define AB_MARKER_NONE 0UL

#include "settings.h"

bool ab_before_A_marker(unsigned long song_position);
bool ab_after_A_marker(unsigned long song_position);
void ab_jump_to_A_marker(void);
void ab_reset_markers(void);
void ab_set_A_marker(unsigned long song_position);
void ab_set_B_marker(unsigned long song_position);
/* These return whether the marker are actually set.
 * The actual positions are returned via output parameter */
bool ab_get_A_marker(unsigned long *song_position);
bool ab_get_B_marker(unsigned long *song_position);

static inline bool ab_repeat_mode_enabled(void)
{
    return global_settings.repeat_mode == REPEAT_AB;
}

#define AB_B_MARKER_WINDOW_DETECT(var, song_position) \
    ((song_position) >= (var) &&                         \
     (song_position) <= (var) + B_MARKER_DETECT_WINDOW)

/* Following is the size of the window in which we'll detect that the B marker
was hit; it must be larger than the frequency (in milliseconds) at which this
function is called otherwise detection of the B marker will be unreliable */

#if CONFIG_CODEC == SWCODEC
/* On swcodec, the worst case seems to be 96kHz with 1024 samples between
 * calls, meaning ~94 calls per second, look within 1/5 of a second */
#define B_MARKER_DETECT_WINDOW 200

/* Time constant to indicate end of track position */
#define AB_POS_END_OF_TRACK ULONG_MAX

extern unsigned long __codec_ab_A_marker;
extern unsigned long __codec_ab_B_marker;
void codec_ab_A_seek(void);

static inline void ab_repeat_init(void)
    {}

static inline bool codec_ab_A_marker_is_set(void)
{
    return __codec_ab_A_marker != AB_MARKER_NONE;
}

static inline bool codec_ab_B_marker_is_set(void)
{
    return __codec_ab_B_marker != AB_MARKER_NONE;
}

static inline bool codec_ab_reached_B_marker(unsigned long song_position)
{
    return codec_ab_B_marker_is_set() &&
           AB_B_MARKER_WINDOW_DETECT(__codec_ab_B_marker, song_position);
}

static inline void codec_ab_position_report(unsigned long position)
{
    if ( ab_repeat_mode_enabled() && codec_ab_reached_B_marker(position) )
        codec_ab_A_seek();
}

static inline bool codec_ab_end_of_track_report(void)
{
    if ( ab_repeat_mode_enabled() )
    {
        codec_ab_A_seek();
        return true;
    }

    return false;
}

void codec_ab_update_markers(unsigned long elapsed);

#else /* !SWCODEC */

/* These functions really need to be inlined for speed */
extern unsigned long ab_A_marker;
extern unsigned long ab_B_marker;

/* we assume that this function will be called on each system tick and derive
the window size from this with a generous margin of error (note: the number
of ticks per second is given by HZ) */
#define B_MARKER_DETECT_WINDOW ((1000/HZ)*10)

static inline bool ab_reached_B_marker(unsigned long song_position)
{
    return ab_B_marker != AB_MARKER_NONE &&
        AB_B_MARKER_WINDOW_DETECT(ab_B_marker, song_position);
}

void ab_repeat_init(void);

#endif /* SWCODEC */

#else /* ndef AB_REPEAT_ENABLE */

#if (CONFIG_CODEC == SWCODEC)
#define PLAYBACK_AB_DECLARE_MARKERS()

static inline bool ab_position_report(unsigned long position)
    { return false; (void)position; }
#endif /* SWCODEC */

static inline bool ab_repeat_mode_enabled(void)
    { return false; }

#endif /* AB_REPEAT_ENABLE */

#endif /* _ABREPEAT_H_ */
