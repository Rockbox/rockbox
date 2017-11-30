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

unsigned long ab_A_marker IDATA_ATTR = AB_MARKER_NONE;
unsigned long ab_B_marker IDATA_ATTR = AB_MARKER_NONE;

static inline bool ab_A_marker_set(void)
{
    return ab_A_marker != AB_MARKER_NONE;
}

static inline bool ab_B_marker_set(void)
{
    return ab_B_marker != AB_MARKER_NONE;
}

static inline void clear_all_markers(void)
{
    ab_A_marker = AB_MARKER_NONE;
    ab_B_marker = AB_MARKER_NONE;
}

#if (CONFIG_CODEC == SWCODEC)
#include "playback.h"
#include "codec_thread.h"

/* following is a fudge factor to help overcome the latency between
the time the user hears the passage they want to mark and the time
they actually press the button; the actual song position is adjusted
by this fudge factor when setting a mark */
#define EAR_TO_HAND_LATENCY_FUDGE 0UL

unsigned long __codec_ab_A_marker = AB_MARKER_NONE;
unsigned long __codec_ab_B_marker = AB_MARKER_NONE;

static inline void update_ab_markers(void)
{
    codec_update_markers();
}

static void playback_ab_seek(unsigned long time)
{
    audio_seek(time, AUDIO_SEEK_SET);
}

static void playback_ab_clear_loops(void)
{
    audio_seek(0, AUDIO_SEEK_CUR);
}

void codec_ab_A_seek(void)
{
    codec_ab_seek(__codec_ab_A_marker);
}

void codec_ab_update_markers(unsigned long elapsed)
{
    unsigned long A = ab_A_marker;
    unsigned long B = ab_B_marker;

    if ( B != AB_MARKER_NONE && A > B )
        B = AB_MARKER_NONE;

    bool A_changed = __codec_ab_A_marker != A;
    bool B_changed = __codec_ab_B_marker != B;

    if ( !A_changed && !B_changed )
        return;

#if 0
    bool A_was_set = codec_ab_A_marker_is_set();
    bool B_was_set = codec_ab_B_marker_is_set();
#endif

    __codec_ab_A_marker = A;
    __codec_ab_B_marker = B;

    if ( ! playback_status() )
        return;

    bool clear = true;

    if ( codec_ab_reached_B_marker(elapsed) )
    {
        playback_ab_seek(A);
        clear = false;
    }
#if 0
    else if ( !B_was_set && !B_changed )
    {
        clear = A_was_set;
    }
#endif /* 0 */

    if ( clear )
        playback_ab_clear_loops();
}

void ab_jump_to_A_marker(void)
{
    playback_ab_seek(ab_A_marker);
}

#else /* !SWCODEC */

/* following is a fudge factor to help overcome the latency between
the time the user hears the passage they want to mark and the time
they actually press the button; the actual song position is adjusted
by this fudge factor when setting a mark */
#define EAR_TO_HAND_LATENCY_FUDGE 200UL

static int ab_audio_event_handler(unsigned short event, unsigned long data)
{
    int rc = AUDIO_EVENT_RC_IGNORED;
    if ( ab_repeat_mode_enabled() )
    {
        switch(event)
        {
            case AUDIO_EVENT_POS_REPORT:
            {
                if ( ! (audio_status() & AUDIO_STATUS_PAUSE) &&
                        ab_reached_B_marker(data) )
                {
                    ab_jump_to_A_marker();
                    rc = AUDIO_EVENT_RC_HANDLED;
                }
                break;
            }
            case AUDIO_EVENT_END_OF_TRACK:
            {
                if ( ab_A_marker_set() && ! ab_B_marker_set() )
                {
                    ab_jump_to_A_marker();
                    rc = AUDIO_EVENT_RC_HANDLED;
                }
                break;
            }
        }
    }
    return rc;
}

void ab_jump_to_A_marker(void)
{
    bool paused = (audio_status() & AUDIO_STATUS_PAUSE) != 0;
    if ( ! paused )
        audio_pause();

    audio_ff_rewind(ab_A_marker);

    if ( ! paused )
        audio_resume();
}

#define update_ab_markers(void) do {} while (0)

void ab_repeat_init(void)
{
    static bool ab_initialized = false;
    if ( ! ab_initialized )
    {
        ab_initialized = true;
        audio_register_event_handler(ab_audio_event_handler,
            AUDIO_EVENT_POS_REPORT | AUDIO_EVENT_END_OF_TRACK );
    }
}

#endif /* SWCODEC */
/* following is the size of the virtual A marker; we pretend that the A marker
is larger than a single instant in order to give the user time to hit PREV again
to jump back to the start of the song; it should be large enough to allow a
reasonable amount of time for the typical user to react */
#define A_MARKER_VIRTUAL_SIZE 1000UL

/* determines if the given song position is earlier than the A mark;
intended for use in handling the jump NEXT and PREV commands */
bool ab_before_A_marker(unsigned long song_position)
{
    return ab_A_marker_set() && (song_position < ab_A_marker);
}

/* determines if the given song position is later than the A mark;
intended for use in handling the jump PREV command */
bool ab_after_A_marker(unsigned long song_position)
{
    return (ab_A_marker != AB_MARKER_NONE)
        && (song_position > (ab_A_marker+A_MARKER_VIRTUAL_SIZE));
}

void ab_set_A_marker(unsigned long song_position)
{
    ab_A_marker = song_position;
#if EAR_TO_HAND_LATENCY_FUDGE
    ab_A_marker = (ab_A_marker >= EAR_TO_HAND_LATENCY_FUDGE) 
        ? (ab_A_marker - EAR_TO_HAND_LATENCY_FUDGE) : 0UL;
#endif
    /* check if markers are out of order */
    if ( (ab_B_marker != AB_MARKER_NONE) && (ab_A_marker > ab_B_marker) )
        ab_B_marker = AB_MARKER_NONE;

    update_ab_markers();
}

void ab_set_B_marker(unsigned long song_position)
{
    ab_B_marker = song_position;
#if EAR_TO_HAND_LATENCY_FUDGE
    ab_B_marker = (ab_B_marker >= EAR_TO_HAND_LATENCY_FUDGE) 
        ? (ab_B_marker - EAR_TO_HAND_LATENCY_FUDGE) : 0UL;
#endif
    /* check if markers are out of order */
    if ( (ab_A_marker != AB_MARKER_NONE) && (ab_B_marker < ab_A_marker) )
        ab_A_marker = AB_MARKER_NONE;

    update_ab_markers();
}

void ab_reset_markers(void)
{
    clear_all_markers();
    update_ab_markers();
}

bool ab_get_A_marker(unsigned long *song_position)
{
    *song_position = ab_A_marker;
    return ab_A_marker_set();
}

bool ab_get_B_marker(unsigned long *song_position)
{
    *song_position = ab_B_marker;
    return ab_B_marker_set();
}

#endif /* AB_REPEAT_ENABLE */
