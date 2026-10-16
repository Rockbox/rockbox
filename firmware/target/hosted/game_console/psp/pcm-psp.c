/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 Mauricio G.
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

#include <stdbool.h>

#include "config.h"
#include "debug.h"
#include "panic.h"

#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_sampr.h"
#include "pcm_mixer.h"
#include "pcm_sink.h"

#include <pspaudio.h>
#include "sys_thread.h"
#include "sys_kernel_hook.h"


/* This driver is based on rockbox pcm-ibasso.c */


/* Audio channel handle */
static int _audio_handle = 1;


/* Bytes left in the Rockbox PCM frame buffer. */
static size_t _pcm_buffer_size = 0;


/* Rockbox PCM frame buffer. */
static const void  *_pcm_buffer = NULL;

/*
    1: PCM thread suspended.
    0: PCM thread running.
    These are used by pcm_play_[lock|unlock] or pcm_play_dma_[start|stop|pause]. These need to be
    separated because of nested calls for locking and stopping.
*/
static volatile int _dma_stopped = 1;
static volatile int _dma_locked  = 1;
static volatile int _dma_shutdown  = 0;


/* Mutex for PCM thread suspend/unsuspend. */
static sysMutex _dma_suspended_mtx;


/* Signal condition for PCM thread suspend/unsuspend. */
static sysCond _dma_suspended_cond;


static int pcm_thread_run(SceSize args, void *argp)
{
    (void) args;
    (void) argp;

    DEBUGF("DEBUG %s: Thread start.\n", __func__);

    while(!AtomicGet(&_dma_shutdown))
    {
        sys_mutex_lock(&_dma_suspended_mtx);
        while((AtomicGet(&_dma_stopped) == 1) || (AtomicGet(&_dma_locked) == 1))
        {
            DEBUGF("DEBUG %s: Playback suspended.\n", __func__);
            sys_cond_wait(&_dma_suspended_cond, &_dma_suspended_mtx);
            DEBUGF("DEBUG %s: Playback resumed.\n", __func__);
        }
        sys_mutex_unlock(&_dma_suspended_mtx);

        if(_pcm_buffer_size == 0)
        {
            /* Retrive a new PCM buffer from Rockbox. */
            if(! pcm_play_dma_complete_callback(PCM_DMAST_OK, &_pcm_buffer, &_pcm_buffer_size))
            {
                DEBUGF("DEBUG %s: No new buffer.\n", __func__);

                sceKernelDelayThread( 10000 );
                continue;
            }
        }
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);

        /* This relies on Rockbox PCM frame buffer size == ALSA PCM frame buffer size. */
        int result = sceAudioOutputPannedBlocking(_audio_handle, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, (void *) _pcm_buffer);
        if( result < 0)
        {
            DEBUGF("sceAudioOutputPannedBlocking: error 0x%x\n", result);

            sceKernelDelayThread( 10000 );
            continue;
        }

        _pcm_buffer_size = 0;

        /*DEBUGF("DEBUG %s: Thread running.\n", __func__);*/
    }

    DEBUGF("DEBUG %s: Thread end.\n", __func__);

    return 0;
}


/* Thread that copies the Rockbox PCM buffer to PSP audio channel. */
static SceUID _pcm_thread;


static void sink_dma_init(void)
{
    /* sys_audio_set_frequency(pcm_sampr); */
    sys_mutex_init(&_dma_suspended_mtx);
    sys_cond_init(&_dma_suspended_cond);

    _audio_handle = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,
                                      MIX_FRAME_SAMPLES,
                                      PSP_AUDIO_FORMAT_STEREO);

    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_stopped, 1);
    AtomicSet(&_dma_locked, 1);
    AtomicSet(&_dma_shutdown, 0);

    /* Create pcm thread in the suspended state. */
    _pcm_thread = sceKernelCreateThread("pcm",
                                        pcm_thread_run,
                                        17,					/* high priority */
                                        32 * 1024, 	/* 32 Kb */
                                        PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU,
                                        NULL);
    sceKernelStartThread(_pcm_thread, 0, 0);
    sys_mutex_unlock(&_dma_suspended_mtx);
}

static void sink_dma_start(const void *addr, size_t size)
{
    _pcm_buffer      = addr;
    _pcm_buffer_size = size;

    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_stopped, 0);
    sys_cond_signal(&_dma_suspended_cond);
    sys_mutex_unlock(&_dma_suspended_mtx);
}

static void sink_dma_stop(void)
{
    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_stopped, 1);
    sys_mutex_unlock(&_dma_suspended_mtx);
}

static void sink_dma_postinit(void)
{
}

static void sink_lock(void)
{
    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_locked, 1);
    sys_mutex_unlock(&_dma_suspended_mtx);
}

static void sink_unlock(void)
{
    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_locked, 0);
    sys_cond_signal(&_dma_suspended_cond);
    sys_mutex_unlock(&_dma_suspended_mtx);
}

static void sink_set_freq_nolock(uint16_t freq)
{
    DEBUGF("sink_set_freq_nolock(%d -> %ld)\n", freq, hw_freq_sampr[freq]);
    int result = sys_audio_set_frequency(hw_freq_sampr[freq]);
    if (result < 0) {
        DEBUGF("sys_audio_set_frequency: error 0x%x\n", result);
    }
}

static void sink_set_freq(uint16_t freq)
{
    sink_lock();
    sink_set_freq_nolock(freq);
    sink_unlock();
}

void pcm_close_device(void)
{
    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_stopped, 0);
    AtomicSet(&_dma_shutdown, 1);

    /* another thread could have the lock and prevent shutdown */
    AtomicSet(&_dma_locked, 0);
    sys_cond_signal(&_dma_suspended_cond);
    sys_mutex_unlock(&_dma_suspended_mtx);

    /* wait thread to finish */
    sceKernelWaitThreadEnd(_pcm_thread, NULL);
    sceAudioChRelease(_audio_handle);

    /* reset audio frequency to default */
    int result = sys_audio_set_frequency(44100);
    if (result < 0) {
        DEBUGF("sys_audio_set_frequency: error 0x%x\n", result);
    }
}

/* moved from drivers/audio/psp.c */
void audiohw_close(void)
{
    pcm_close_device();
}

struct pcm_sink builtin_pcm_sink = {
    .caps = {
        .samprs       = hw_freq_sampr,
        .num_samprs   = HW_NUM_FREQ,
        .default_freq = HW_FREQ_DEFAULT,
    },
    .ops = {
        .init     = sink_dma_init,
        .postinit = sink_dma_postinit,
        .set_freq = sink_set_freq,
        .lock     = sink_lock,
        .unlock   = sink_unlock,
        .play     = sink_dma_start,
        .stop     = sink_dma_stop,
    },
};
