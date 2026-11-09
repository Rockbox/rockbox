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
static LightLock _dma_suspended_mtx;


/* Signal condition for PCM thread suspend/unsuspend. */
static CondVar _dma_suspended_cond;


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

/* mic samples per output frame, FP_SHIFT fixed point; set at start */
static u32 _pcm_resample_step = FP_ONE;

/* a filled chunk waiting for the callback (recording was locked) */
static bool _chunk_ready = false;

/* Mic ring buffer: 3 s at 32.7 kHz mono, as in the devkitPro mic example.
 * The recording thread may fall behind while Rockbox has interrupts
 * disabled or an FS request blocks; this is how long it can before audio
 * is lost. */
#define MIC_BUFFER_SIZE 0x30000

/* poll interval while waiting for the mic to fill a chunk */
#define REC_POLL_NS (10 * 1000000ULL)

static int record_and_resample_stereo_fixed_point(void)
{
    const u32 sample_bytes = sizeof(s16);
    u32 write_offset = micGetLastSampleOffset();

    u32 available =
        (write_offset >= _mic_read_offset) ? (write_offset - _mic_read_offset) : (_mic_buffer_size - _mic_read_offset + write_offset);
    available /= sample_bytes;

    const u32 step = _pcm_resample_step;

    u32 needed = ((u32)_pcm_frame_samples * step >> FP_SHIFT) + 2;
    if (available < needed) {
        return 0;
    }

    /* Fell so far behind that the mic is about to overwrite unread data:
     * skip to the newest data. This loses audio. */
    if (available > (_mic_buffer_size / sample_bytes) * 3 / 4) {
        logf("rec overrun: dropped %lu samples\n", available);
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

        /* 15 bit fraction: (sb - sa) spans 17 bits, the product must fit
         * in 32 */
        s32 sample = sa + (((sb - sa) * (s32)(frac >> 1)) >> (FP_SHIFT - 1));

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
        LightLock_Lock(&_dma_suspended_mtx);
        while(((AtomicGet(&_dma_state) == DMA_STOPPED) || (AtomicGet(&_dma_locked) == 1)) &&
              !AtomicGet(&_dma_shutdown))
        {
            DEBUGF("DEBUG %s: Recording suspended.\n", __func__);
            CondVar_Wait(&_dma_suspended_cond, &_dma_suspended_mtx);
            DEBUGF("DEBUG %s: Recording resumed.\n", __func__);
        }
        LightLock_Unlock(&_dma_suspended_mtx);

        if(AtomicGet(&_dma_state) == DMA_STOP_PENDING)
        {
            AtomicSet(&_dma_state, DMA_STOPPED);
        }
        else if(AtomicGet(&_dma_state) == DMA_RECORDING)
        {
            /* try to get some new data from the microphone */
            if (!_chunk_ready && record_and_resample_stereo_fixed_point() > 0)
                _chunk_ready = true;

            if (_chunk_ready) {
                /* libctru thread: run Rockbox callbacks as an emulated irq.
                 * pcm_rec_lock() may have been called since the check
                 * above: check again where it can't change, else keep the
                 * chunk for after pcm_rec_unlock() */
                bool delivered = false, more = false;
                sim_enter_irq_handler();
                if (!AtomicGet(&_dma_locked) &&
                    AtomicGet(&_dma_state) == DMA_RECORDING) {
                    _chunk_ready = false;
                    delivered = true;
                    more = pcm_rec_dma_complete_callback(PCM_DMAST_OK, &_pcm_buffer, &_pcm_buffer_size);
                    if (more)
                        pcm_rec_dma_status_callback(PCM_DMAST_STARTED);
                }
                sim_exit_irq_handler();

                if (delivered && more) {
                    _pcm_frame_samples = _pcm_buffer_size / (2 * sizeof(int16_t));
                    /* catch up: the next chunk may be waiting already */
                    continue;
                }
                else if (delivered) {
                    logf("Recording stopped ...\r");
                    AtomicSet(&_dma_state, DMA_STOPPED);
                }
            }

            /* wait for more captured data */
            svcSleepThread(REC_POLL_NS);
        }
    }

    if(AtomicGet(&_dma_state) == DMA_SHUTDOWN_PENDING) {
        AtomicSet(&_dma_state, DMA_SHUTDOWN);
    }

    DEBUGF("DEBUG %s: Thread end.\n", __func__);
}

void pcm_rec_lock(void)
{
    /* Like masking the DMA interrupt: the callback runs as an emulated
     * interrupt, so taking the irq lock waits for one in progress, and
     * the flag keeps later ones out. The thread never holds the LightLock
     * while entering a handler, so this order can't deadlock. */
    int oldlevel = disable_irq_save();
    LightLock_Lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_locked, 1);
    LightLock_Unlock(&_dma_suspended_mtx);
    restore_irq(oldlevel);
}

void pcm_rec_unlock(void)
{
    LightLock_Lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_locked, 0);
    CondVar_Signal(&_dma_suspended_cond);
    LightLock_Unlock(&_dma_suspended_mtx);
}

void pcm_rec_dma_init(void)
{
    logf("pcm_rec_dma_init()\n");
    u32 micbuf_size = MIC_BUFFER_SIZE;
    _mic_buffer = memalign(0x1000, micbuf_size);
    Result res = micInit(_mic_buffer, micbuf_size);
    if(R_FAILED(res)) {
        logf("micInit failed: 0x%lX", res);
    }

    /* TODO: the MIC hardware loses ~2 samples (~60 us) each time sampling
     * wraps around this ring, i.e. once every ~3 s. Measured with a test
     * tone in the raw ring data, so the driver reads it correctly; the
     * samples never reach the buffer. Inaudible, ignored for now. */
    _mic_buffer_size = micGetSampleDataSize();

    LightLock_Init(&_dma_suspended_mtx);
    CondVar_Init(&_dma_suspended_cond);

    LightLock_Lock(&_dma_suspended_mtx);
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
    LightLock_Unlock(&_dma_suspended_mtx);
}

void pcm_rec_dma_close(void)
{
    logf("pcm_rec_dma_close()\n");
    LightLock_Lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_state, DMA_SHUTDOWN_PENDING);
    AtomicSet(&_dma_shutdown, 1);
    /* wake the thread if it is parked, and let it take the lock to exit */
    CondVar_Broadcast(&_dma_suspended_cond);
    LightLock_Unlock(&_dma_suspended_mtx);

    /* wait thread to finish */
    threadJoin(_pcm_rec_thread, UINT64_MAX);
    threadFree(_pcm_rec_thread);

    /* the MIC service writes into the buffer until sampling stops and the
     * shared memory is unmapped: free it only after micExit() */
    MICU_StopSampling();
    micExit();

    if (_mic_buffer) {
        free(_mic_buffer);
        _mic_buffer = NULL;
        _mic_buffer_size = 0;
    }
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
    _chunk_ready = false;

    logf("_pcm_frame_samples = %d\n", _pcm_frame_samples);

    /* hardware real frequency value to resample from */
    _mic_rate = realSampleRate(_pcm_encode_rate);
    /* rounded from the exact rate (32728.498 Hz, not 32728) */
    _pcm_resample_step = (u32)(_mic_rate * FP_ONE / _pcm_encode_rate + 0.5f);

    /* start sampling microphone data */
    MICU_StartSampling(MICU_ENCODING_PCM16_SIGNED,              /* encoding */
                       MICU_GetSampleRate(_pcm_encode_rate),    /* sample rate */
                       0,                                       /* offset */
                       _mic_buffer_size,                        /* buffer size */
                       true);                                   /* loop */

    /* before waking the thread: it runs at a higher priority */
    _mic_read_offset = micGetLastSampleOffset();

    LightLock_Lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_state, DMA_RECORDING);
    CondVar_Signal(&_dma_suspended_cond);
    LightLock_Unlock(&_dma_suspended_mtx);
}

void pcm_rec_dma_stop(void)
{
    logf("pcm_rec_dma_stop()\n");
    LightLock_Lock(&_dma_suspended_mtx);
    AtomicSet(&_dma_state, DMA_STOP_PENDING);
    LightLock_Unlock(&_dma_suspended_mtx);

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
