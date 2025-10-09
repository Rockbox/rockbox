/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Nick Lanham
 * Copyright (C) 2010 by Thomas Martitz
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

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#include "autoconf.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "config.h"
#include "debug.h"
#include "sound.h"
#include "audiohw.h"
#include "system.h"
#include "panic.h"

#ifdef HAVE_RECORDING
#include "audiohw.h"
#ifdef HAVE_SPDIF_IN
#include "spdif.h"
#endif
#endif

#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_sampr.h"
#include "pcm_mixer.h"

#include <3ds/ndsp/ndsp.h>
#include <3ds/ndsp/channel.h>
#include <3ds/services/dsp.h>
#include <3ds/synchronization.h>
#include <3ds/allocator/linear.h>

/*#define LOGF_ENABLE*/
#include "logf.h"

#ifdef DEBUG
extern bool debug_audio;
#endif

extern const char      *audiodev;

/* Bytes left in the Rockbox PCM frame buffer. */
static size_t _pcm_buffer_size = 0;


/* Rockbox PCM frame buffer. */
static const void  *_pcm_buffer = NULL;

/*
    1: PCM thread suspended.
    0: PCM thread running.
    These are used by pcm_play_[lock|unlock] or pcm_play_dma_[start|stop|pause]. These need to be
    separated because of nested calls for suspending and stopping.
*/
static volatile int _dsp_enabled  = 1;
static volatile int _pcm_shutdown = 0;


/* Mutex for PCM thread suspend/unsuspend. */
static RecursiveLock _pcm_lock_mtx;         /* audio device mutex */
static LightEvent    _dsp_callback_event;   /* dsp callback synchronization flag */

static Thread _pcm_thread;
static int _pcm_thread_id = -1;

/* DSP wave buffers */
static ndspWaveBuf _dsp_wave_bufs[3];
static s16 *_dsp_audio_buffer = NULL;

static inline bool is_in_audio_thread(int audio_thread_id)
{
    /* The device thread locks the same mutex, but not through the public API.
       This check is in case the application, in the audio callback,
       tries to lock the thread that we've already locked from the
       device thread...just in case we only have non-recursive mutexes. */
    if ( (sys_thread_id() == audio_thread_id)) {
        return true;
    }

    return false;
}

void pcm_play_lock(void)
{
    if (!is_in_audio_thread(_pcm_thread_id)) {
        RecursiveLock_Lock(&_pcm_lock_mtx);
    }
}

void pcm_play_unlock(void)
{
     if (!is_in_audio_thread(_pcm_thread_id)) {
        RecursiveLock_Unlock(&_pcm_lock_mtx);
    }
}

static void pcm_write_to_soundcard(const void *pcm_buffer, size_t pcm_buffer_size, ndspWaveBuf *dsp_buffer)
{
    s16 *buffer = dsp_buffer->data_pcm16;
    memcpy(buffer, pcm_buffer, pcm_buffer_size);

    dsp_buffer->nsamples = pcm_buffer_size / 2 / sizeof(s16);
    ndspChnWaveBufAdd(0, dsp_buffer);
    DSP_FlushDataCache(buffer, pcm_buffer_size);
}

bool fill_buffer(ndspWaveBuf *dsp_buffer)
{
    if(_pcm_buffer_size == 0)
    {
        /* Retrive a new PCM buffer from Rockbox. */
        if(!pcm_play_dma_complete_callback(PCM_DMAST_OK, &_pcm_buffer, &_pcm_buffer_size))
        {
            /* DEBUGF("DEBUG %s: No new buffer.\n", __func__); */
            svcSleepThread(10000);
            return false;
        }
    }
    pcm_play_dma_status_callback(PCM_DMAST_STARTED);

    /* This relies on Rockbox PCM frame buffer size == ALSA PCM frame buffer size. */
    pcm_write_to_soundcard(_pcm_buffer, _pcm_buffer_size, dsp_buffer);
    _pcm_buffer_size = 0;

    return true;
}

void pcm_thread_run(void* nothing)
{
    (void) nothing;

    DEBUGF("DEBUG %s: Thread start.\n", __func__);
    
    _pcm_thread_id = sys_thread_id();

    while(!AtomicGet(&_pcm_shutdown))
    {
        RecursiveLock_Lock(&_pcm_lock_mtx);
        for(size_t i = 0; i < ARRAY_SIZE(_dsp_wave_bufs); ++i) {
            if(_dsp_wave_bufs[i].status != NDSP_WBUF_DONE) {
                continue;
            }
            
            if(!fill_buffer(&_dsp_wave_bufs[i])) {
                continue;
            }
        }
        RecursiveLock_Unlock(&_pcm_lock_mtx);

        // Wait for a signal that we're needed again before continuing,
        // so that we can yield to other things that want to run
        // (Note that the 3DS uses cooperative threading)
        LightEvent_Wait(&_dsp_callback_event);
    }

    DEBUGF("DEBUG %s: Thread end.\n", __func__);
}

void dsp_callback(void *const nul_) {
    (void)nul_;

    if(AtomicGet(&_pcm_shutdown)) {
        return;
    }
    
    LightEvent_Signal(&_dsp_callback_event);
}

static void pcm_dma_apply_settings_nolock(void)
{
    ndspChnReset(0);
    
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);

    ndspChnSetRate(0, pcm_sampr);
    ndspChnSetFormat(0, NDSP_FORMAT_STEREO_PCM16);
    /* ndspChnSetInterp(0, NDSP_INTERP_POLYPHASE); */
    /* ndspChnSetInterp(0, NDSP_INTERP_NONE); */
    ndspChnSetInterp(0, NDSP_INTERP_LINEAR);

    float mix[12];
    memset(mix, 0, sizeof(mix));
    mix[0] = 1.0;
    mix[1] = 1.0;
    ndspChnSetMix(0, mix);

    memset(_dsp_wave_bufs, 0, sizeof(_dsp_wave_bufs) * ARRAY_SIZE(_dsp_wave_bufs));
    
    const size_t wave_buffer_size = MIX_FRAME_SAMPLES * 2 * sizeof(s16);
    size_t buffer_size = wave_buffer_size * ARRAY_SIZE(_dsp_wave_bufs);

    _dsp_audio_buffer = (s16 *)linearAlloc(buffer_size);

    s16 *buffer = _dsp_audio_buffer;
    for (unsigned i = 0; i < ARRAY_SIZE(_dsp_wave_bufs); i++) {
        _dsp_wave_bufs[i].data_vaddr = buffer;
        _dsp_wave_bufs[i].status     = NDSP_WBUF_DONE;
        
        buffer += wave_buffer_size / sizeof(buffer[0]);
    }
    
    ndspChnSetPaused(0, true);
    ndspSetCallback(dsp_callback, NULL);

    AtomicSet(&_pcm_shutdown, 0);
    AtomicSet(&_dsp_enabled, 1);

    // Start the thread, passing our opusFile as an argument.
    _pcm_thread = threadCreate(pcm_thread_run,
                               NULL,
                               32 * 1024,       /* 32kB stack size */
                               0x18,            /* high priority */ 
                              -1,              /* run on any core */ false);
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    _pcm_buffer = addr;
    _pcm_buffer_size = size;

    RecursiveLock_Lock(&_pcm_lock_mtx);
    ndspChnSetPaused(0, false);
    RecursiveLock_Unlock(&_pcm_lock_mtx);
}

void pcm_play_dma_stop(void)
{
    RecursiveLock_Lock(&_pcm_lock_mtx);
    ndspChnSetPaused(0, true);
    RecursiveLock_Unlock(&_pcm_lock_mtx);
}

/* TODO: implement recording */
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

void pcm_play_dma_init(void)
{
    Result ndsp_init_res = ndspInit();
    if (R_FAILED(ndsp_init_res)) {
        if ((R_SUMMARY(ndsp_init_res) == RS_NOTFOUND) && (R_MODULE(ndsp_init_res) == RM_DSP)) {
            logf("DSP init failed: dspfirm.cdc missing!");
        } else {
            logf("DSP init failed. Error code: 0x%lX", ndsp_init_res);
        }
        return;
    }

    RecursiveLock_Init(&_pcm_lock_mtx);
    LightEvent_Init(&_dsp_callback_event, RESET_ONESHOT);
}

void pcm_play_dma_postinit(void)
{
}

void pcm_dma_apply_settings(void)
{
    pcm_play_lock();
    pcm_dma_apply_settings_nolock();
    pcm_play_unlock();
}

void pcm_close_device(void)
{
    RecursiveLock_Lock(&_pcm_lock_mtx);
    AtomicSet(&_pcm_shutdown, 1);
    LightEvent_Signal(&_dsp_callback_event);

    threadJoin(_pcm_thread, UINT64_MAX);
    threadFree(_pcm_thread);
    _pcm_thread_id = -1;
    RecursiveLock_Unlock(&_pcm_lock_mtx);
    
    ndspSetCallback(NULL, NULL);
    
    ndspChnReset(0);
    if (_dsp_audio_buffer != NULL) {
        linearFree(_dsp_audio_buffer);
        _dsp_audio_buffer = NULL;
    }

    ndspExit();
}

/* moved from drivers/audio/ctru.c */
void audiohw_close(void)
{
    pcm_close_device();
}
