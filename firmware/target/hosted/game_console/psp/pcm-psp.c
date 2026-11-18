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
#include <pspkernel.h>
#include "sys_thread.h"
#include "sys_kernel_hook.h"
#include "system.h"     /* sim_enter_irq_handler() */

/* The PCM thread is an OS thread outside the Rockbox scheduler: it calls
 * back into Rockbox as an interrupt handler, the way a DMA interrupt would. */
#define PCM_IRQ_ENTER() sim_enter_irq_handler()
#define PCM_IRQ_EXIT()  sim_exit_irq_handler()

static int _audio_handle = 1;
static size_t _pcm_buffer_size = 0;
static const void  *_pcm_buffer = NULL;

/*
    1: PCM thread suspended.
    0: PCM thread running.
    These are used by pcm_play_[lock|unlock] or pcm_play_dma_[start|stop|pause]. These need to be
    separated because of nested calls for locking and stopping.
    Like a masked DMA interrupt, the thread doesn't call back into Rockbox
    while either is set.
*/
static volatile int _dma_stopped = 1;
static volatile int _dma_locked  = 1;
static volatile int _dma_shutdown  = 0;

/* Wakes the PCM thread when it is suspended. A semaphore stays signalled
   until it is consumed, so a wakeup between checking the flags and waiting
   is not lost. */
static SceUID _dma_wakeup;

/* PCM_DMAST_STARTED makes the mixer fill its other buffer, so it is reported
   only once the output has accepted a buffer fetched by this thread: then
   the buffer before it has been played. Not for the first buffer, which
   comes from sink_dma_start(): its caller has already prepared the next one.
   While masked, it waits like a pending interrupt. */
static bool _started_pending = false;

static inline bool dma_suspended(void)
{
    return AtomicGet(&_dma_stopped) == 1 || AtomicGet(&_dma_locked) == 1;
}

static int pcm_thread_run(SceSize args, void *argp)
{
    (void) args;
    (void) argp;

    DEBUGF("DEBUG %s: Thread start.\n", __func__);

    while(!AtomicGet(&_dma_shutdown))
    {
        while(dma_suspended() && !AtomicGet(&_dma_shutdown))
        {
            DEBUGF("DEBUG %s: Playback suspended.\n", __func__);
            sceKernelWaitSema(_dma_wakeup, 1, NULL);
            DEBUGF("DEBUG %s: Playback resumed.\n", __func__);
        }
        if(AtomicGet(&_dma_shutdown))
            break;

        PCM_IRQ_ENTER();
        /* Locked or stopped while entering: the interrupt would have been
           masked, so don't call back into Rockbox now */
        if(dma_suspended())
        {
            PCM_IRQ_EXIT();
            continue;
        }

        if(_started_pending)
        {
            _started_pending = false;
            pcm_play_dma_status_callback(PCM_DMAST_STARTED);
        }

        bool fetched = false;
        if(_pcm_buffer_size == 0)
        {
            /* Retrive a new PCM buffer from Rockbox. */
            if(!pcm_play_dma_complete_callback(PCM_DMAST_OK, &_pcm_buffer, &_pcm_buffer_size))
            {
                PCM_IRQ_EXIT();
                DEBUGF("DEBUG %s: No new buffer.\n", __func__);
                sceKernelDelayThread( 10000 );
                continue;
            }
            fetched = true;
        }

        /* Take the buffer now: playback may restart with a new one while
           this thread is blocked below */
        const void *buffer = _pcm_buffer;
        _pcm_buffer_size = 0;
        PCM_IRQ_EXIT();

        /* This relies on Rockbox PCM frame buffer size == ALSA PCM frame buffer size. */
        int result = sceAudioOutputPannedBlocking(_audio_handle, PSP_AUDIO_VOLUME_MAX, PSP_AUDIO_VOLUME_MAX, (void *) buffer);
        if( result < 0)
        {
            DEBUGF("sceAudioOutputPannedBlocking: error 0x%x\n", result);
            sceKernelDelayThread( 10000 );
            continue;
        }

        if(fetched)
            _started_pending = true;
    }

    DEBUGF("DEBUG %s: Thread end.\n", __func__);
    return 0;
}

/* Thread that copies the Rockbox PCM buffer to the PSP audio channel. */
static SceUID _pcm_thread;

static void sink_dma_init(void)
{
    /* sys_audio_set_frequency(pcm_sampr); */
    _dma_wakeup = sceKernelCreateSema("pcm_wakeup", 0, 0, 1, NULL);

    _audio_handle = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,
                                      MIX_FRAME_SAMPLES,
                                      PSP_AUDIO_FORMAT_STEREO);

    AtomicSet(&_dma_stopped, 1);
    AtomicSet(&_dma_locked, 1);
    AtomicSet(&_dma_shutdown, 0);

    /* Create pcm thread in the suspended state. */
    _pcm_thread = sceKernelCreateThread("pcm",
                                        pcm_thread_run,
                                        17,         /* high priority */
                                        32 * 1024,  /* 32 Kb */
                                        PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU,
                                        NULL);
    sceKernelStartThread(_pcm_thread, 0, 0);
}

/* Rockbox calls these with the PCM "interrupt" masked (pcm_play_lock()), so
 * the thread never sees a half-written buffer */
static void sink_dma_start(const void *addr, size_t size)
{
    _pcm_buffer      = addr;
    _pcm_buffer_size = size;
    _started_pending = false;

    AtomicSet(&_dma_stopped, 0);
    sceKernelSignalSema(_dma_wakeup, 1);
}

static void sink_dma_stop(void)
{
    AtomicSet(&_dma_stopped, 1);
}

static void sink_dma_postinit(void)
{
}

static void sink_lock(void)
{
    AtomicSet(&_dma_locked, 1);
}

static void sink_unlock(void)
{
    AtomicSet(&_dma_locked, 0);
    sceKernelSignalSema(_dma_wakeup, 1);
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
    AtomicSet(&_dma_shutdown, 1);
    sceKernelSignalSema(_dma_wakeup, 1);

    /* wait thread to finish */
    sceKernelWaitThreadEnd(_pcm_thread, NULL);
    sceKernelDeleteThread(_pcm_thread);
    sceKernelDeleteSema(_dma_wakeup);

    sceAudioChRelease(_audio_handle);

    /* reset audio frequency to default */
    int result = sys_audio_set_frequency(44100);
    if (result < 0) {
        DEBUGF("sys_audio_set_frequency: error 0x%x\n", result);
    }
}

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
