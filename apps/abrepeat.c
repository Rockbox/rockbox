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

#include "config.h"
#include "abrepeat.h"

#ifdef AB_REPEAT_ENABLE

unsigned int ab_A_marker IDATA_ATTR = AB_MARKER_NONE;
unsigned int ab_B_marker IDATA_ATTR = AB_MARKER_NONE;


static inline bool ab_A_marker_set(void)
{
    return ab_A_marker != AB_MARKER_NONE;
}

static inline bool ab_B_marker_set(void)
{
    return ab_B_marker != AB_MARKER_NONE;
}


void ab_end_of_track_report(void)
{
    if ( ab_A_marker_set() && ! ab_B_marker_set() )
    {
        ab_jump_to_A_marker();
    }
}

void ab_repeat_init(void)
{
    static bool ab_initialized = false;
    if ( ! ab_initialized )
    {
        ab_initialized = true;
    }
}

/* determines if the given song position is earlier than the A mark;
intended for use in handling the jump NEXT and PREV commands */
bool ab_before_A_marker(unsigned int song_position)
{
    return (ab_A_marker != AB_MARKER_NONE)
        && (song_position < ab_A_marker);
}

/* determines if the given song position is later than the A mark;
intended for use in handling the jump PREV command */
bool ab_after_A_marker(unsigned int song_position)
{
/* following is the size of the virtual A marker; we pretend that the A marker
is larger than a single instant in order to give the user time to hit PREV again
to jump back to the start of the song; it should be large enough to allow a
reasonable amount of time for the typical user to react */
#define A_MARKER_VIRTUAL_SIZE 1000
    return (ab_A_marker != AB_MARKER_NONE)
        && (song_position > (ab_A_marker+A_MARKER_VIRTUAL_SIZE));
}

void ab_jump_to_A_marker(void)
{
    audio_ff_rewind(ab_A_marker);
}

void ab_reset_markers(void)
{
    ab_A_marker = AB_MARKER_NONE;
    ab_B_marker = AB_MARKER_NONE;
}

/* following is a fudge factor to help overcome the latency between
the time the user hears the passage they want to mark and the time
they actually press the button; the actual song position is adjusted
by this fudge factor when setting a mark */
#define EAR_TO_HAND_LATENCY_FUDGE 200

void ab_set_A_marker(unsigned int song_position)
{
    ab_A_marker = song_position;
    ab_A_marker = (ab_A_marker >= EAR_TO_HAND_LATENCY_FUDGE) 
        ? (ab_A_marker - EAR_TO_HAND_LATENCY_FUDGE) : 0;
    /* check if markers are out of order */
    if ( (ab_B_marker != AB_MARKER_NONE) && (ab_A_marker > ab_B_marker) )
        ab_B_marker = AB_MARKER_NONE;
}

void ab_set_B_marker(unsigned int song_position)
{
    ab_B_marker = song_position;
    ab_B_marker = (ab_B_marker >= EAR_TO_HAND_LATENCY_FUDGE) 
        ? (ab_B_marker - EAR_TO_HAND_LATENCY_FUDGE) : 0;
    /* check if markers are out of order */
    if ( (ab_A_marker != AB_MARKER_NONE) && (ab_B_marker < ab_A_marker) )
        ab_A_marker = AB_MARKER_NONE;
}

bool ab_get_A_marker(unsigned *song_position)
{
    *song_position = ab_A_marker;
    return ab_A_marker_set();
}

bool ab_get_B_marker(unsigned *song_position)
{
    *song_position = ab_B_marker;
    return ab_B_marker_set();
}

#endif /* AB_REPEAT_ENABLE */
