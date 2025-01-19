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
#include "config.h"
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "usb.h"
#include "pcm.h"
#include "sound.h"
#include "pcmbuf.h"
#include "appevents.h"
#include "audio_thread.h"
#ifdef AUDIO_HAVE_RECORDING
#include "pcm_record.h"
#endif
#include "codec_thread.h"
#include "voice_thread.h"
#include "talk.h"
#include "settings.h"

/* Macros to enable logf for queues
   logging on SYS_TIMEOUT can be disabled */
#ifdef SIMULATOR
/* Define this for logf output of all queuing except SYS_TIMEOUT */
#define AUDIO_LOGQUEUES
/* Define this to logf SYS_TIMEOUT messages */
/*#define AUDIO_LOGQUEUES_SYS_TIMEOUT*/
#endif

#ifdef AUDIO_LOGQUEUES
#define LOGFQUEUE logf
#else
#define LOGFQUEUE(...)
#endif

bool audio_is_initialized = false;

/* Event queues */
struct event_queue audio_queue SHAREDBSS_ATTR;
static struct queue_sender_list audio_queue_sender_list SHAREDBSS_ATTR;

/* Audio thread */
static long audio_stack[(DEFAULT_STACK_SIZE + 0x1000)/sizeof(long)];
static const char audio_thread_name[] = "audio";
unsigned int audio_thread_id = 0;

static void NORETURN_ATTR audio_thread(void)
{
    struct queue_event ev;
    ev.id = Q_NULL; /* something not in switch below */

    pcm_postinit();

    while (1)
    {
        switch (ev.id)
        {
        /* Starts the playback engine branch */
        case Q_AUDIO_PLAY:
            LOGFQUEUE("audio < Q_AUDIO_PLAY");
            audio_playback_handler(&ev);
            continue;

        /* Playback has to handle these, even if not playing */
        case Q_AUDIO_REMAKE_AUDIO_BUFFER:
#ifdef HAVE_DISK_STORAGE
        case Q_AUDIO_UPDATE_WATERMARK:
#endif
            audio_playback_handler(&ev);
            break;

#ifdef AUDIO_HAVE_RECORDING
        /* Starts the recording engine branch */
        case Q_AUDIO_INIT_RECORDING:
            LOGFQUEUE("audio < Q_AUDIO_INIT_RECORDING");
            audio_recording_handler(&ev);
            continue;
#endif

        /* All return upon USB */
        case SYS_USB_CONNECTED:
            LOGFQUEUE("audio < SYS_USB_CONNECTED");
            voice_stop();
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            usb_wait_for_disconnect(&audio_queue);
            break;
        }

        queue_wait(&audio_queue, &ev);
    }
}

void audio_voice_event(unsigned short id, void *data)
{
    (void)id;
    /* Make audio play softly while voice is speaking */
    pcmbuf_soft_mode(*(bool *)data);
}

void audio_queue_post(long id, intptr_t data)
{
    queue_post(&audio_queue, id, data);
}

intptr_t audio_queue_send(long id, intptr_t data)
{
    return queue_send(&audio_queue, id, data);
}

/* Return the playback and recording status */
int audio_status(void)
{
    return playback_status()
#ifdef AUDIO_HAVE_RECORDING
            | pcm_rec_status()
#endif
    ;
}

/* Clear all accumulated audio errors for playback and recording */
void audio_error_clear(void)
{
#ifdef AUDIO_HAVE_RECORDING
    pcm_rec_error_clear();
#endif
}

/** -- Startup -- **/

/* Initialize the audio system - called from init() in main.c */
void INIT_ATTR audio_init(void)
{
    /* Can never do this twice */
    if (audio_is_initialized)
    {
        logf("audio: already initialized");
        return;
    }

    logf("audio: initializing");

    /* Initialize queues before giving control elsewhere in case it likes
       to send messages. Thread creation will be delayed however so nothing
       starts running until ready if something yields such as talk_init. */
    queue_init(&audio_queue, true);
    codec_thread_init();

    /* This thread does buffer, so match its priority */
    audio_thread_id = create_thread(audio_thread, audio_stack,
                  sizeof(audio_stack), 0, audio_thread_name
                  IF_PRIO(, MIN(PRIORITY_BUFFERING, PRIORITY_USER_INTERFACE))
                  IF_COP(, CPU));

    queue_enable_queue_send(&audio_queue, &audio_queue_sender_list,
                            audio_thread_id);

    playback_init();
#ifdef AUDIO_HAVE_RECORDING
    recording_init();
#endif

    add_event(VOICE_EVENT_IS_PLAYING, audio_voice_event);

   /* Probably safe to say */
    audio_is_initialized = true;

    sound_settings_apply();
}
