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

#if CONFIG_CODEC == SWCODEC
/* Including the code for fast previews is entirely optional since it
   does add two more mp3entry's - for certain targets it may be less
   beneficial such as flash-only storage */
#if MEMORYSIZE > 2
#define AUDIO_FAST_SKIP_PREVIEW
#endif

#endif /* CONFIG_CODEC == SWCODEC */

#ifdef HAVE_ALBUMART

#include "bmp.h"
#include "metadata.h"
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

struct bufopen_bitmap_data {
    struct dim *dim;
    struct mp3_albumart *embedded_albumart;
};

#endif

/* Functions */
bool audio_is_thread_ready(void);
int audio_track_count(void);
long audio_filebufused(void);
void audio_pre_ff_rewind(void);
void audio_skip(int direction);
void audio_hard_stop(void); /* Stops audio from serving playback */

void audio_set_cuesheet(int enable);
#ifdef HAVE_CROSSFADE
void audio_set_crossfade(int enable);
#endif

enum
{
    AUDIO_WANT_PLAYBACK = 0,
    AUDIO_WANT_VOICE,
};
bool audio_restore_playback(int type); /* Restores the audio buffer to handle the requested playback */
size_t audio_get_filebuflen(void);
bool audio_buffer_state_trashed(void);

/* Automatic transition? Only valid to call during the track change events,
   otherwise the result is undefined. */
bool audio_automatic_skip(void);

/* Define one constant that includes recording related functionality */
#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
#define AUDIO_HAVE_RECORDING
#endif

enum {
    Q_NULL = 0,                 /* reserved */

    /* -> audio */
    Q_AUDIO_PLAY = 1,
    Q_AUDIO_STOP,
    Q_AUDIO_PAUSE,
    Q_AUDIO_SKIP,
    Q_AUDIO_PRE_FF_REWIND,
    Q_AUDIO_FF_REWIND,
    Q_AUDIO_FLUSH,
    Q_AUDIO_DIR_SKIP,

    /* pcmbuf -> audio */
    Q_AUDIO_TRACK_CHANGED,

    /* audio -> audio */
    Q_AUDIO_FILL_BUFFER,        /* continue buffering next track */

    /* buffering -> audio */
    Q_AUDIO_BUFFERING,          /* some buffer event */
    Q_AUDIO_FINISH_LOAD_TRACK,  /* metadata is buffered */
    Q_AUDIO_HANDLE_FINISHED,    /* some other type is buffered */

    /* codec -> audio (*) */
    Q_AUDIO_CODEC_SEEK_COMPLETE,
    Q_AUDIO_CODEC_COMPLETE,

    /* audio -> codec */
    Q_CODEC_LOAD,
    Q_CODEC_RUN,
    Q_CODEC_PAUSE,
    Q_CODEC_SEEK,
    Q_CODEC_STOP,
    Q_CODEC_UNLOAD,


    /*- miscellanous -*/
#ifdef AUDIO_HAVE_RECORDING
    /* -> codec */
    Q_AUDIO_LOAD_ENCODER,       /* load an encoder for recording */
#endif
    /* -> codec */
    Q_CODEC_DO_CALLBACK,


    /*- settings -*/

#ifdef HAVE_DISK_STORAGE
    /* -> audio */
    Q_AUDIO_UPDATE_WATERMARK,   /* buffering watermark needs updating */
#endif
    /* -> audio */
    Q_AUDIO_REMAKE_AUDIO_BUFFER, /* buffer needs to be reinitialized */
};

/* (*) If you change these, you must check audio_clear_track_notifications
       in playback.c for correctness */

#endif /* _PLAYBACK_H */
