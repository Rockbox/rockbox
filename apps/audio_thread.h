/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005-2007 Miika Pekkarinen
 * Copyright (C) 2007-2008 Nicolas Pennequin
 * Copyright (C) 2011-2013 Michael Sevakis
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
#ifndef AUDIO_THREAD_H
#define AUDIO_THREAD_H

/* Define one constant that includes recording related functionality */
#if defined(HAVE_RECORDING) && !defined(SIMULATOR)
#define AUDIO_HAVE_RECORDING
#endif

enum
{
    Q_NULL = 0,                 /* reserved */

    /* -> audio */
    Q_AUDIO_PLAY,
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

    /* -> codec */
    Q_CODEC_DO_CALLBACK,

    /* -> recording */
#ifdef HAVE_RECORDING
    Q_AUDIO_INIT_RECORDING,
    Q_AUDIO_CLOSE_RECORDING,
    Q_AUDIO_RECORDING_OPTIONS,
    Q_AUDIO_RECORD,
    Q_AUDIO_RECORD_STOP,
    Q_AUDIO_RECORD_PAUSE,
    Q_AUDIO_RECORD_RESUME,
    Q_AUDIO_RECORD_FLUSH,
#endif

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

void audio_init(void);
void playback_init(void);
unsigned int playback_status(void);

void audio_playback_handler(struct queue_event *ev);
#ifdef AUDIO_HAVE_RECORDING
void audio_recording_handler(struct queue_event *ev);
#endif

/** --- audio_queue helpers --- **/
void audio_queue_post(long id, intptr_t data);
intptr_t audio_queue_send(long id, intptr_t data);

#endif /* AUDIO_THREAD_H */
