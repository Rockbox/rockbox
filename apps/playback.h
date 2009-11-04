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
#include <stdlib.h>
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
size_t audio_get_filebuflen(void);
void audio_pcmbuf_position_callback(size_t size) ICODE_ATTR;
void audio_post_track_change(void);
int get_audio_hid(void);
int *get_codec_hid(void);
void audio_set_prev_elapsed(unsigned long setting);
bool audio_is_playing(void);
bool audio_is_paused(void);

/* Define one constant that includes recording related functionality */
#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
#define AUDIO_HAVE_RECORDING
#endif

enum {
    Q_NULL = 0,
    Q_AUDIO_PLAY = 1,
    Q_AUDIO_STOP,
    Q_AUDIO_PAUSE,
    Q_AUDIO_SKIP,
    Q_AUDIO_PRE_FF_REWIND,
    Q_AUDIO_FF_REWIND,
    Q_AUDIO_CHECK_NEW_TRACK,
    Q_AUDIO_FLUSH,
    Q_AUDIO_TRACK_CHANGED,
    Q_AUDIO_DIR_SKIP,
    Q_AUDIO_POSTINIT,
    Q_AUDIO_FILL_BUFFER,
    Q_AUDIO_FINISH_LOAD,
    Q_CODEC_REQUEST_COMPLETE,
    Q_CODEC_REQUEST_FAILED,

    Q_CODEC_LOAD,
    Q_CODEC_LOAD_DISK,

#ifdef AUDIO_HAVE_RECORDING
    Q_ENCODER_LOAD_DISK,
    Q_ENCODER_RECORD,
#endif

    Q_CODEC_DO_CALLBACK,
};

#if CONFIG_CODEC == SWCODEC
void audio_next_dir(void);
void audio_prev_dir(void);
#else
#define audio_next_dir() ({ })
#define audio_prev_dir() ({ })
#endif

#endif
