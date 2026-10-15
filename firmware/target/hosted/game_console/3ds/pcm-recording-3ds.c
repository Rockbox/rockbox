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
#include <malloc.h>

#include <3ds/ndsp/ndsp.h>
#include <3ds/ndsp/channel.h>
#include <3ds/services/dsp.h>
#include <3ds/services/mic.h>
#include <3ds/synchronization.h>
#include <3ds/allocator/linear.h>

#include "sys_atomic.h"
#include "sys_thread.h"

#include "config.h"
#include "debug.h"
#include "sound.h"
#include "audiohw.h"
#include "system.h"
#include "panic.h"
#include "audiohw.h"

#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_sampr.h"
#include "pcm_mixer.h"
#include "pcm_sink.h"

/*#define LOGF_ENABLE */
#include "logf.h"

enum {
    DMA_RECORDING,
    DMA_STOP_PENDING,
    DMA_STOPPED,
    DMA_SHUTDOWN_PENDING,
    DMA_SHUTDOWN,
};


/*
    1: PCM thread suspended.
    0: PCM thread running.
    These are used by pcm_play_[lock|unlock] or pcm_play_dma_[start|stop|pause]. These need to be
    separated because of nested calls for locking and stopping.
*/
static volatile int _dma_state = DMA_STOPPED;
static volatile int _dma_locked  = 1;
static volatile int _dma_shutdown  = 0;


/* Mutex for PCM thread suspend/unsuspend. */
static sysMutex _dma_suspended_mtx;


/* Signal condition for PCM thread suspend/unsuspend. */
static sysCond *_dma_suspended_cond;


/* Bytes left in the Rockbox PCM frame buffer. */
static size_t _pcm_buffer_size = 0;


/* Rockbox PCM frame buffer. */
static void  *_pcm_buffer = NULL;


/* MIC audio buffer */
int _pcm_encode_rate = 32000;
static int _pcm_frame_samples;
// MICU_SAMPLE_RATE_16360 is actually 16364.479Hz.
//static float _mic_rate = 16364.479f;
static float _mic_rate = 32728.498f;

static u8* _mic_buffer = NULL;
static size_t _mic_buffer_size = 0;
static u32 _mic_read_offset = 0;

static Thread _pcm_rec_thread = NULL;

/* Since the 3ds uses custom sampling rates for recording,
   we will use a fast resampling algorithm to match rockbox
   recording sampling rates */

#define FP_SHIFT 16
#define FP_ONE   (1 << FP_SHIFT)

static int _pcm_resample_pos = 0;

static int record_and_resample_stereo_fixed_point(void)
{
    const u32 sample_bytes = sizeof(s16);
    u32 write_offset = micGetLastSampleOffset();

    u32 available =
        (write_offset >= _mic_read_offset) ? (write_offset - _mic_read_offset) : (_mic_buffer_size - _mic_read_offset + write_offset);
    available /= sample_bytes;

    const u32 step = ((u32)_mic_rate << FP_SHIFT) / _pcm_encode_rate;

    u32 needed = ((u32)_pcm_frame_samples * step >> FP_SHIFT) + 2;
    if (available < needed) {
        return 0;
    }

    if (available > (_mic_buffer_size / sample_bytes) / 2) {
        _mic_read_offset = write_offset;
        _pcm_resample_pos = 0;
        return 0;
    }

    s16 *dst = (s16 *)_pcm_buffer;
    const s16 *src = (const s16 *)_mic_buffer;
    const u32 total_samples = _mic_buffer_size / sample_bytes;
    const u32 start = _mic_read_offset / sample_bytes;
    u32 pos = _pcm_resample_pos;

    for (int i = 0; i < _pcm_frame_samples; i++) {
        u32 index = pos >> FP_SHIFT;
        u32 frac  = pos & (FP_ONE - 1);

        u32 a = start + index;

        if (a >= total_samples)
            a -= total_samples;

        u32 b = a + 1;

        if (b >= total_samples)
            b = 0;

        s32 sa = src[a];
        s32 sb = src[b];

        s32 sample = sa + (((sb - sa) * (s32)frac) >> FP_SHIFT);

        dst[i * 2 + 0] = (s16)sample;
        dst[i * 2 + 1] = (s16)sample;

        pos += step;
    }

    u32 consumed = pos >> FP_SHIFT;
    _pcm_resample_pos = pos & (FP_ONE - 1);
    _mic_read_offset = (_mic_read_offset + consumed * sample_bytes) % _mic_buffer_size;

    _pcm_buffer_size -= _pcm_frame_samples * 4;

    return _pcm_frame_samples;
}

static void pcm_rec_thread_run(void* nothing)
{
    (void) nothing;

    DEBUGF("DEBUG %s: Thread start.\n", __func__);

    while(!AtomicGet(&_dma_shutdown))
    {
        sys_mutex_lock(&_dma_suspended_mtx);
        while((AtomicGet(&_dma_state) == DMA_STOPPED) || (AtomicGet(&_dma_locked) == 1))
        {
            DEBUGF("DEBUG %s: Recording suspended.\n", __func__);
            sys_cond_wait(_dma_suspended_cond, &_dma_suspended_mtx);
            DEBUGF("DEBUG %s: Recording resumed.\n", __func__);
        }
        sys_mutex_unlock(&_dma_suspended_mtx);

        if(AtomicGet(&_dma_state) == DMA_STOP_PENDING)
        {
            AtomicSet(&_dma_state, DMA_STOPPED);
        }
        else if(AtomicGet(&_dma_state) == DMA_RECORDING)
        {
            /* try to get some new data from the microphone */
            if (record_and_resample_stereo_fixed_point() > 0) {
                if (pcm_rec_dma_complete_callback(PCM_DMAST_OK, &_pcm_buffer, &_pcm_buffer_size)) {
                    pcm_rec_dma_status_callback(PCM_DMAST_STARTED);

                    _pcm_frame_samples = _pcm_buffer_size / (2 * sizeof(int16_t));
                }
                else {
                    logf("Recording stopped ...\r");
                    AtomicSet(&_dma_state, DMA_STOPPED);
                }
            }

            /* wait for more captured data */
            svcSleepThread(20 * 1000000ULL);
        }
    }

    if(AtomicGet(&_dma_state) == DMA_SHUTDOWN_PENDING) {
        AtomicSet(&_dma_state, DMA_SHUTDOWN);
    }

    DEBUGF("DEBUG %s: Thread end.\n", __func__);
}

void pcm_rec_lock(void)
{
    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_locked, 1);
    sys_mutex_unlock(&_dma_suspended_mtx);
}

void pcm_rec_unlock(void)
{
    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_locked, 0);
    sys_cond_signal(_dma_suspended_cond);
    sys_mutex_unlock(&_dma_suspended_mtx);
}

void pcm_rec_dma_init(void)
{
    logf("pcm_rec_dma_init()\n");
    u32 micbuf_size = 0x10000;
    _mic_buffer = memalign(0x1000, micbuf_size);
    Result res = micInit(_mic_buffer, micbuf_size);
    if(R_FAILED(res)) {
        logf("micInit failed: 0x%lX", res);
    }

    _mic_buffer_size = micGetSampleDataSize();

    sys_mutex_init(&_dma_suspended_mtx);
    _dma_suspended_cond = sys_cond_create();

    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_state, DMA_STOPPED);
    AtomicSet(&_dma_locked, 1);
    AtomicSet(&_dma_shutdown, 0);

    /* create thread in suspended state */
    _pcm_rec_thread = threadCreate(pcm_rec_thread_run,
                                   NULL,
                                   32 * 1024,       /* 32kB stack size */
                                   0x18,            /* high priority */
                                   -1,              /* run on any core */
                                   false);
    sys_mutex_unlock(&_dma_suspended_mtx);
}

void pcm_rec_dma_close(void)
{
    logf("pcm_rec_dma_close()\n");
    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_state, DMA_SHUTDOWN_PENDING);
    AtomicSet(&_dma_shutdown, 1);

    /* wait thread to finish */
    threadJoin(_pcm_rec_thread, UINT64_MAX);
    threadFree(_pcm_rec_thread);
    sys_mutex_unlock(&_dma_suspended_mtx);

    if (_mic_buffer) {
        free(_mic_buffer);
        _mic_buffer = NULL;
        _mic_buffer_size = 0;
    }

    micExit();
}

MICU_SampleRate MICU_GetSampleRate(int freq)
{
    switch(freq) {
    case 8000:
        return MICU_SAMPLE_RATE_8180;
    case 11000:
        return MICU_SAMPLE_RATE_10910;
    case 16000:
        return MICU_SAMPLE_RATE_16360;
    case 32000:
    default:
        return MICU_SAMPLE_RATE_32730;
    };
}

float realSampleRate(int freq)
{
    switch(freq) {
    case 8000:
        return 8182.1245f;
    case 11000:
        return 10909.499f;
    case 16000:
        return 16364.479f;;
    case 32000:
    default:
        return 32728.498f;
    };
}

void pcm_rec_dma_start(void *start, size_t size)
{
    logf("pcm_rec_dma_start(%d)\n", size);
    _pcm_buffer      = start;
    _pcm_buffer_size = size;

    _pcm_frame_samples = _pcm_buffer_size / (2 * sizeof(int16_t));
    _pcm_resample_pos = 0;

    logf("_pcm_frame_samples = %d\n", _pcm_frame_samples);

    /* hardware real frequency value to resample from */
    _mic_rate = realSampleRate(_pcm_encode_rate);

    /* start sampling microphone data */
    MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED,              /* encoding */
                       MICU_GetSampleRate(_pcm_encode_rate),    /* sample rate */
                       0,                                       /* offset */
                       _mic_buffer_size,                        /* buffer size */
                       true);                                   /* loop */

    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_state, DMA_RECORDING);
    sys_cond_signal(_dma_suspended_cond);
    sys_mutex_unlock(&_dma_suspended_mtx);

    _mic_read_offset = micGetLastSampleOffset();
}

void pcm_rec_dma_stop(void)
{
    logf("pcm_rec_dma_stop()\n");
    sys_mutex_lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_state, DMA_STOP_PENDING);
    sys_mutex_unlock(&_dma_suspended_mtx);

    MICU_StopSampling();
}

const void * pcm_rec_dma_get_peak_buffer(void)
{
    /* use 1024 bytes of audio buffer to read peak data */
    uintptr_t addr = (uintptr_t)(_pcm_buffer + 1024);
    return (void*)((addr + 3) & ~3);
}

void audiohw_set_recvol(int left, int right, int type)
{
    (void)right; (void)type;
    logf("audiohw_set_recvol: %d dB\n", left);

    u8 hw_gain = (u8)((left * 115 + 30) / 60);
    MICU_SetGain(hw_gain);
}
