/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Miika Pekkarinen
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

#ifndef _PLAYBACK_H
#define _PLAYBACK_H

#include <stdbool.h>
#include "config.h"

#ifdef HAVE_ALBUMART

#include "bmp.h"
/*
 * Returns the handle id of the buffered albumart for the given slot id
 **/
int playback_current_aa_hid(int slot);

/*
 * Hands out an albumart slot for buffering albumart using the size
 * int the passed dim struct, it copies the data of dim in order to
 * be safe to be reused for other code
 *
 * The slot may be reused if other code calls this with the same dimensions
 * in dim, so if you change dim release and claim a new slot
 *
 * Save to call from other threads */
int playback_claim_aa_slot(struct dim *dim);

/*
 * Releases the albumart slot with given id
 * 
 * Save to call from other threads */
void playback_release_aa_slot(int slot);
#endif

/* Functions */
const char *get_codec_filename(int cod_spec);
void voice_wait(void);
bool audio_is_thread_ready(void);
int audio_track_count(void);
long audio_filebufused(void);
void audio_pre_ff_rewind(void);
void audio_set_crossfade(int type);
void audio_skip(int direction);
void audio_hard_stop(void); /* Stops audio from serving playback */

enum
{
    AUDIO_WANT_PLAYBACK = 0,
    AUDIO_WANT_VOICE,
};
bool audio_restore_playback(int type); /* Restores the audio buffer to handle the requested playback */
void codec_thread_do_callback(void (*fn)(void),
                              unsigned int *codec_thread_id);


#if CONFIG_CODEC == SWCODEC /* This #ifdef is better here than gui/wps.c */
extern void audio_next_dir(void);
extern void audio_prev_dir(void);
#else
#define audio_next_dir() ({ })
#define audio_prev_dir() ({ })
#endif

#endif


