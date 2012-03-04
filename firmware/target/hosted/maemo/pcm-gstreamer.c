/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 by Thomas Jarosch
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


#include "autoconf.h"

#include <stdbool.h>
#include "config.h"
#include "debug.h"
#include "sound.h"
#include "audiohw.h"
#include "system.h"
#include "settings.h"

#include "playback.h"
#include "kernel.h"

#include <pthread.h>
#include <SDL.h>
#include <glib.h>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <linux/types.h>

/* Maemo5: N900 specific libplayback support */
#include <libplayback/playback.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include "maemo-thread.h"

#ifdef HAVE_RECORDING
#include "audiohw.h"
#ifdef HAVE_SPDIF_IN
#include "spdif.h"
#endif
#endif

#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_sampr.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#ifdef DEBUG
#include <stdio.h>
extern bool debug_audio;
#endif

#if CONFIG_CODEC == SWCODEC

/* Declarations for libplayblack */
pb_playback_t *playback = NULL;
void playback_state_req_handler(pb_playback_t *pb,
                                                      enum pb_state_e req_state,
                                                      pb_req_t *ext_req,
                                                      void *data);
void playback_state_req_callback(pb_playback_t *pb,
            enum pb_state_e granted_state,
            const char *reason,
            pb_req_t *req,
            void *data);
bool playback_granted = false;

/* Gstreamer related vars */
GstCaps *gst_audio_caps = NULL;
GstElement *gst_pipeline = NULL;
GstElement *gst_appsrc = NULL;
GstElement *gst_volume = NULL;
GstElement *gst_pulsesink = NULL;
GstBus *gst_bus = NULL;
static int bus_watch_id = 0;
GMainLoop *pcm_loop = NULL;

static const void* pcm_data = NULL;
static size_t pcm_data_size = 0;

static int audio_locked = 0;
static pthread_mutex_t audio_lock_mutex = PTHREAD_MUTEX_INITIALIZER;
static int inside_feed_data = 0;

/*
 * mutex lock/unlock wrappers neatness' sake
 */
static inline void lock_audio(void)
{
    pthread_mutex_lock(&audio_lock_mutex);
}

static inline void unlock_audio(void)
{
    pthread_mutex_unlock(&audio_lock_mutex);
}

void pcm_play_lock(void)
{
    if (++audio_locked == 1)
        lock_audio();
}

void pcm_play_unlock(void)
{
    if (--audio_locked == 0)
        unlock_audio();
}

void pcm_dma_apply_settings(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    pcm_data = addr;
    pcm_data_size = size;

    if (playback_granted)
    {
        /* Start playing now */
        if (!inside_feed_data)
            gst_element_set_state (GST_ELEMENT(gst_pipeline), GST_STATE_PLAYING);
        else
            DEBUGF("ERROR: dma_start called while inside feed_data\n");
    } else
    {
        /* N900: Request change to playing state */
        pb_playback_req_state   (playback,
                                                PB_STATE_PLAY,
                                                playback_state_req_callback,
                                                NULL);
    }
}

void pcm_play_dma_stop(void)
{
    if (inside_feed_data)
        g_signal_emit_by_name (gst_appsrc, "end-of-stream", NULL);
    else
        gst_element_set_state (GST_ELEMENT(gst_pipeline), GST_STATE_NULL);
}

void pcm_play_dma_pause(bool pause)
{
    if (inside_feed_data)
    {
        if (pause)
            g_signal_emit_by_name (gst_appsrc, "end-of-stream", NULL);
        else
            DEBUGF("ERROR: Called dma_pause(0) while inside feed_data\n");
    } else
    {
        if (pause)
            gst_element_set_state (GST_ELEMENT(gst_pipeline), GST_STATE_NULL);
        else
            gst_element_set_state (GST_ELEMENT(gst_pipeline), GST_STATE_PLAYING);
    }
}

size_t pcm_get_bytes_waiting(void)
{
    return pcm_data_size;
}

static void feed_data(GstElement * appsrc, guint size_hint, void *unused)
{
    (void)size_hint;
    (void)unused;

    lock_audio();

    /* Make sure we don't trigger a gst_element_set_state() call
       from inside gstreamer's stream thread as it will deadlock */
    inside_feed_data = 1;

    if (pcm_play_dma_complete_callback(PCM_DMAST_OK, &pcm_data, &pcm_data_size))
    {
        GstBuffer *buffer = gst_buffer_new ();
        GstFlowReturn ret;

        GST_BUFFER_DATA (buffer) = (__u8 *)pcm_data;
        GST_BUFFER_SIZE (buffer) = pcm_data_size;

        g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);
        gst_buffer_unref (buffer);

        if (ret != 0)
            DEBUGF("push-buffer error result: %d\n", ret);

        pcm_play_dma_status_callback(PCM_DMAST_STARTED);
    } else
    {
        DEBUGF("feed_data: No Data.\n");
        g_signal_emit_by_name (appsrc, "end-of-stream", NULL);
    }

    inside_feed_data = 0;

    unlock_audio();
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    uintptr_t addr = (uintptr_t)pcm_data;
    *count = pcm_data_size / 4;
    return (void *)((addr + 2) & ~3);
}


static gboolean
gst_bus_message (GstBus * bus, GstMessage * message, void *unused)
{
    (void)bus;
    (void)unused;

    DEBUGF("    [gst] got BUS message %s\n",
        gst_message_type_get_name (GST_MESSAGE_TYPE (message)));

    switch (GST_MESSAGE_TYPE (message)) {
        case GST_MESSAGE_ERROR:
        {
        GError *err;
        gchar *debug;
        gst_message_parse_error (message, &err, &debug);

        DEBUGF("[gst] Received error: Src: %s, msg: %s\n", GST_MESSAGE_SRC_NAME(message), err->message);

        g_error_free (err);
        g_free (debug);
        }

        g_main_loop_quit (pcm_loop);
        break;
        case GST_MESSAGE_EOS:
        gst_element_set_state (GST_ELEMENT(gst_pipeline), GST_STATE_NULL);
        break;
    case GST_MESSAGE_STATE_CHANGED:
    {
        GstState old_state, new_state;

        gst_message_parse_state_changed (message, &old_state, &new_state, NULL);
        DEBUGF("[gst] Element %s changed state from %s to %s.\n",
            GST_MESSAGE_SRC_NAME(message),
            gst_element_state_get_name (old_state),
            gst_element_state_get_name (new_state));
        break;
        }
        default:
        break;
    }

    return TRUE;
}

void maemo_configure_appsrc(void)
{
    /* Block push-buffer until there is enough room */
    g_object_set (G_OBJECT(gst_appsrc), "block", TRUE, NULL);

    g_object_set(G_OBJECT(gst_appsrc), "format", GST_FORMAT_BYTES, NULL);

    gst_audio_caps = gst_caps_new_simple("audio/x-raw-int", "width", G_TYPE_INT, (gint)16, "depth", G_TYPE_INT, (gint)16, "channels" ,G_TYPE_INT, (gint)2,
                                "signed",G_TYPE_BOOLEAN,1,
                                "rate",G_TYPE_INT,44100,"endianness",G_TYPE_INT,(gint)1234,NULL);

    g_object_set (G_OBJECT(gst_appsrc), "caps", gst_audio_caps, NULL);

    gst_app_src_set_stream_type(GST_APP_SRC(gst_appsrc),
        GST_APP_STREAM_TYPE_STREAM);

    /* configure the appsrc, we will push data into the appsrc from the
    * mainloop. */
    g_signal_connect (gst_appsrc, "need-data", G_CALLBACK (feed_data), NULL);
}

/* Init libplayback: Grant access rights to
   play audio while the phone is in silent mode */
void maemo_init_libplayback(void)
{
    DBusConnection *session_bus_raw = (DBusConnection*)osso_get_dbus_connection(maemo_osso_ctx);

    playback = pb_playback_new_2(session_bus_raw,
                                               PB_CLASS_MEDIA,
                                               PB_FLAG_AUDIO,
                                               PB_STATE_STOP,
                                               playback_state_req_handler,
                                               NULL);

    pb_playback_set_stream(playback, "Playback Stream");
}

/**
 * Gets called by the policy framework if an important
 * event arrives: Incoming calls etc.
 */
void maemo_tell_rockbox_to_stop_audio(void)
{
    sim_enter_irq_handler();
    queue_broadcast(SYS_CALL_INCOMING, 0);
    sim_exit_irq_handler();

    osso_system_note_infoprint(maemo_osso_ctx, "Stopping rockbox playback", NULL);
}

void playback_state_req_handler(pb_playback_t *pb,
                                                      enum pb_state_e req_state,
                                                      pb_req_t *ext_req,
                                                      void *data)
{
    (void)pb;
    (void)ext_req;
    (void)data;

    DEBUGF("External state change request: state: %s, data: %p\n",
            pb_state_to_string(req_state), data);

    if (req_state == PB_STATE_STOP && playback_granted)
    {
        DEBUGF("Stopping playback, might be an incoming call\n");

        playback_granted = false;
        maemo_tell_rockbox_to_stop_audio();
    }
}

/**
 * Callback for our own state change request.
 */
void playback_state_req_callback(pb_playback_t *pb, enum pb_state_e granted_state, const char *reason, pb_req_t *req, void *data)
{
    (void)data;
    (void)reason;

    DEBUGF("State request callback: granted_state: %s, reason: %s\n",
                pb_state_to_string(granted_state), reason);

    /* We are allowed to play audio */
    if (granted_state == PB_STATE_PLAY)
    {
        DEBUGF("Playback granted. Start playing...\n");
        playback_granted = true;
        if (!inside_feed_data)
            gst_element_set_state (GST_ELEMENT(gst_pipeline), GST_STATE_PLAYING);
    } else
    {
        DEBUGF("Can't start playing. Throwing away play request\n");

        playback_granted = false;
        maemo_tell_rockbox_to_stop_audio();
    }

    pb_playback_req_completed(pb, req);
}

void pcm_play_dma_init(void)
{
    maemo_init_libplayback();

    GMainContext *ctx = g_main_loop_get_context(maemo_main_loop);
    pcm_loop = g_main_loop_new (ctx, true);

    gst_init (NULL, NULL);

    gst_pipeline = gst_pipeline_new ("rockbox");

    gst_appsrc = gst_element_factory_make ("appsrc", NULL);
    gst_volume = gst_element_factory_make ("volume", NULL);
    gst_pulsesink = gst_element_factory_make ("pulsesink", NULL);

    /* Connect elements */
    gst_bin_add_many (GST_BIN (gst_pipeline),
                        gst_appsrc, gst_volume, gst_pulsesink, NULL);
    gst_element_link_many (gst_appsrc, gst_volume, gst_pulsesink, NULL);

    /* Connect to gstreamer bus of the pipeline */
    gst_bus = gst_pipeline_get_bus (GST_PIPELINE (gst_pipeline));
    bus_watch_id = gst_bus_add_watch (gst_bus, (GstBusFunc) gst_bus_message, NULL);

    maemo_configure_appsrc();
}

void pcm_shutdown_gstreamer(void)
{
    /* Try to stop playing */
    gst_element_set_state (GST_ELEMENT(gst_pipeline), GST_STATE_NULL);

    /* Make sure we are really stopped. This should return almost instantly,
       so we wait up to ten seconds and just continue otherwise */
    gst_element_get_state (GST_ELEMENT(gst_pipeline), NULL, NULL, GST_SECOND * 10);

    g_source_remove (bus_watch_id);
    g_object_unref(gst_bus);
    gst_bus = NULL;

    gst_object_unref (gst_pipeline);
    gst_pipeline = NULL;

    /* Shutdown libplayback and gstreamer */
    pb_playback_destroy (playback);
    gst_deinit();

    g_main_loop_quit(pcm_loop);
    g_main_loop_unref (pcm_loop);

    pthread_mutex_destroy(&audio_lock_mutex);
}

void pcm_play_dma_postinit(void)
{
}

void pcm_set_mixer_volume(int volume)
{
    /* gstreamer volume range is from 0.00 to 1.00 */
    gdouble gst_vol = (gdouble)(volume - VOLUME_MIN) / (gdouble)VOLUME_RANGE;

    g_object_set (G_OBJECT(gst_volume), "volume", gst_vol, NULL);
}


#ifdef HAVE_RECORDING
void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_start(void *start, size_t size)
{
    (void)start;
    (void)size;
}

void pcm_rec_dma_stop(void)
{
}

const void * pcm_rec_dma_get_peak_buffer(void)
{
    return NULL;
}

void audiohw_set_recvol(int left, int right, int type)
{
    (void)left;
    (void)right;
    (void)type;
}

#ifdef HAVE_SPDIF_IN
unsigned long spdif_measure_frequency(void)
{
    return 0;
}
#endif

#endif /* HAVE_RECORDING */

#endif /* CONFIG_CODEC == SWCODEC */
