/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2010 Thomas Martitz
 * Copyright (c) 2020 Solomon Peachy
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

/*
 * Based, but heavily modified, on the example given at
 * http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html
 *
 * This driver uses the so-called unsafe async callback method.
 *
 * To make the async callback safer, an alternative stack is installed, since
 * it's run from a signal hanlder (which otherwise uses the user stack).
 *
 * TODO: Rewrite this to properly use multithreading and/or direct mmap()
 */

#include "autoconf.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <alsa/asoundlib.h>

//#define LOGF_ENABLE

#include "system.h"
#include "debug.h"
#include "kernel.h"
#include "panic.h"

#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_mixer.h"
#include "pcm_sampr.h"
#include "pcm_sink.h"
#include "audiohw.h"
#include "pcm-alsa.h"
#include "fixedpoint.h"

#include "logf.h"

#if defined(HAVE_EROSQ_LINUX_CODEC)
#include "erosqlinux_codec.h"
#endif

#include <pthread.h>
#include <signal.h>

/* plughw:0,0 works with both, however "default" is recommended.
 * default doesnt seem to work with async callback but doesn't break
 * with multple applications running */
#define DEFAULT_PLAYBACK_DEVICE "plughw:0,0"
#define DEFAULT_CAPTURE_DEVICE "default"

#if MIX_FRAME_SAMPLES < 512
#error "MIX_FRAME_SAMPLES needs to be at least 512!"
#elif MIX_FRAME_SAMPLES < 1024
#warning "MIX_FRAME_SAMPLES <1024 may cause dropouts!"
#endif

/* PCM_DC_OFFSET_VALUE is a workaround for eros q hardware quirk */
#if !defined(PCM_DC_OFFSET_VALUE)
# define PCM_DC_OFFSET_VALUE 0
#endif

static const snd_pcm_access_t access_ = SND_PCM_ACCESS_RW_INTERLEAVED; /* access mode */
#if defined(HAVE_ALSA_32BIT)
static const snd_pcm_format_t format = SND_PCM_FORMAT_S32_LE;    /* sample format */
typedef int32_t sample_t;
#else
static const snd_pcm_format_t format = SND_PCM_FORMAT_S16;    /* sample format */
typedef int16_t sample_t;
#endif
static const int channels = 2;                                /* count of channels */
static unsigned int real_sample_rate;
static unsigned int last_sample_rate;

static snd_pcm_t *handle = NULL;
static snd_pcm_sframes_t buffer_size;
static snd_pcm_sframes_t period_size;
static sample_t *frames = NULL;

static const void  *pcm_data = 0;
static size_t       pcm_size = 0;

static unsigned int xruns = 0;

static snd_async_handler_t *ahandler = NULL;
static pthread_mutex_t pcm_mtx;
static long signal_stack[SIGSTKSZ/sizeof(long)];
/* ALSA "type bluetooth" PCM does not support snd_async_add_pcm_handler (ENOSYS). */
static bool pcm_poll_playback;
static volatile bool pcm_poll_run;
static pthread_t pcm_poll_tid;
static bool pcm_poll_tid_valid;

static void sink_lock(void);
static void sink_unlock(void);
static void sink_set_freq_nolock(uint16_t freq);

static const char *playback_dev = DEFAULT_PLAYBACK_DEVICE;

#ifdef HAVE_RECORDING
static void *pcm_data_rec = DEFAULT_CAPTURE_DEVICE;
static const char *capture_dev = NULL;
static snd_pcm_stream_t current_alsa_mode;  /* SND_PCM_STREAM_PLAYBACK / _CAPTURE */
#endif

static const char *current_alsa_device;

static bool pcm_playback_uses_bt(void)
{
    return playback_dev && strncmp(playback_dev, "pcm.", 4) == 0;
}

static snd_pcm_format_t pcm_playback_format(void)
{
    if (pcm_playback_uses_bt())
        return SND_PCM_FORMAT_S16_LE;
#if defined(HAVE_ALSA_32BIT)
    return SND_PCM_FORMAT_S32_LE;
#else
    return SND_PCM_FORMAT_S16;
#endif
}

static size_t pcm_frame_sample_bytes(void)
{
    if (pcm_playback_uses_bt())
        return sizeof(int16_t);
#if defined(HAVE_ALSA_32BIT)
    return sizeof(int32_t);
#else
    return sizeof(int16_t);
#endif
}

void pcm_alsa_set_playback_device(const char *device)
{
    playback_dev = device;
}

#ifdef HAVE_RECORDING
void pcm_alsa_set_capture_device(const char *device)
{
    capture_dev = device;
}
#endif

static int set_hwparams(snd_pcm_t *handle, unsigned long sampr)
{
    int err;
    unsigned int srate;
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_malloc(&params);

    /* Size playback buffers based on sample rate.

       Buffer size must be at least 4x period size!

       Note these are in FRAMES, and are sized to be about 8.5ms
       for the buffer and 2.1ms for the period
     */
    if (pcm_playback_uses_bt()) {
        /* ALSA bluetooth plugin: minimal buffering for A2DP transport + pause sync.
           128 frames @ 44.1kHz ~ 2.9ms period, ~8.7ms buffer (3 periods). */
        period_size = 128;
        buffer_size = period_size * 3;
    } else if (sampr > SAMPR_96) {
        buffer_size = MIX_FRAME_SAMPLES * 4 * 4;
        period_size = MIX_FRAME_SAMPLES * 4;
    } else if (sampr > SAMPR_48) {
        buffer_size = MIX_FRAME_SAMPLES * 2 * 4;
        period_size = MIX_FRAME_SAMPLES * 2;
    } else {
        buffer_size = MIX_FRAME_SAMPLES * 4;
        period_size = MIX_FRAME_SAMPLES;
    }

    /* choose all parameters */
    err = snd_pcm_hw_params_any(handle, params);
    if (err < 0)
    {
        panicf("Broken configuration for playback: no configurations available: %s", snd_strerror(err));
        goto error;
    }
    /* set the interleaved read/write format */
    err = snd_pcm_hw_params_set_access(handle, params, access_);
    if (err < 0)
    {
        panicf("Access type not available for playback: %s", snd_strerror(err));
        goto error;
    }
    /* set the sample format */
    err = snd_pcm_hw_params_set_format(handle, params, pcm_playback_format());
    if (err < 0)
    {
        logf("Sample format not available for playback: %s", snd_strerror(err));
        goto error;
    }
    /* set the count of channels */
    err = snd_pcm_hw_params_set_channels(handle, params, channels);
    if (err < 0)
    {
        logf("Channels count (%i) not available for playbacks: %s", channels, snd_strerror(err));
        goto error;
    }
    /* set the stream rate */
    srate = sampr;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &srate, 0);
    if (err < 0)
    {
        logf("Rate %luHz not available for playback: %s", sampr, snd_strerror(err));
        goto error;
    }
    real_sample_rate = srate;
    if (!pcm_playback_uses_bt() && real_sample_rate != sampr)
    {
        logf("Rate doesn't match (requested %luHz, get %dHz)", sampr, real_sample_rate);
        err = -EINVAL;
        goto error;
    }
    if (pcm_playback_uses_bt())
        logf("BT PCM rate %uHz (requested %lu)", real_sample_rate, sampr);

    /* set the buffer size */
    err = snd_pcm_hw_params_set_buffer_size_near(handle, params, &buffer_size);
    if (err < 0)
    {
        logf("Unable to set buffer size %ld for playback: %s", buffer_size, snd_strerror(err));
        goto error;
    }

    /* set the period size */
    err = snd_pcm_hw_params_set_period_size_near (handle, params, &period_size, NULL);
    if (err < 0)
    {
        logf("Unable to set period size %ld for playback: %s", period_size, snd_strerror(err));
        goto error;
    }

    if (frames) free(frames);
    frames = calloc(1, period_size * channels * pcm_frame_sample_bytes());

    /* write the parameters to device */
    err = snd_pcm_hw_params(handle, params);
    if (err < 0)
    {
        logf("Unable to set hw params for playback: %s", snd_strerror(err));
        goto error;
    }

    err = 0; /* success */
error:
    snd_pcm_hw_params_free(params);
    return err;
}

/* Set sw params: playback start threshold and low buffer watermark */
static int set_swparams(snd_pcm_t *handle)
{
    int err;

    snd_pcm_sw_params_t *swparams;
    snd_pcm_sw_params_malloc(&swparams);

    /* get the current swparams */
    err = snd_pcm_sw_params_current(handle, swparams);
    if (err < 0)
    {
        logf("Unable to determine current swparams for playback: %s", snd_strerror(err));
        goto error;
    }
    /* DAC: half buffer before start. BT: one period for lower latency. */
    {
        snd_pcm_uframes_t start_thresh = pcm_playback_uses_bt()
            ? (snd_pcm_uframes_t)period_size
            : (snd_pcm_uframes_t)(buffer_size / 2);
        err = snd_pcm_sw_params_set_start_threshold(handle, swparams, start_thresh);
    }
    if (err < 0)
    {
        logf("Unable to set start threshold mode for playback: %s", snd_strerror(err));
        goto error;
    }
    /* BT: wake poll thread as soon as space is available */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams,
            pcm_playback_uses_bt() ? 1 : (unsigned int)period_size);
    if (err < 0)
    {
        logf("Unable to set avail min for playback: %s", snd_strerror(err));
        goto error;
    }
    /* write the parameters to the playback device */
    err = snd_pcm_sw_params(handle, swparams);
    if (err < 0)
    {
        logf("Unable to set sw params for playback: %s", snd_strerror(err));
        goto error;
    }

    err = 0; /* success */
error:
    snd_pcm_sw_params_free(swparams);
    return err;
}

#if defined(HAVE_ALSA_32BIT)
/* Multiplicative factors applied to each sample */
static int32_t dig_vol_mult_l = 0;
static int32_t dig_vol_mult_r = 0;

void pcm_set_mixer_volume(int vol_db_l, int vol_db_r)
{
    dig_vol_mult_l = fp_factor(fp_div(vol_db_l, 10, 16), 16);
    dig_vol_mult_r = fp_factor(fp_div(vol_db_r, 10, 16), 16);
}
#endif

/* copy pcm samples to a spare buffer, suitable for snd_pcm_writei() */
static bool copy_frames(bool first)
{
    ssize_t nframes, frames_left = period_size;
    bool new_buffer = false;

    while (frames_left > 0)
    {
        if (!pcm_size)
        {
            new_buffer = true;
#ifdef HAVE_RECORDING
            switch (current_alsa_mode)
            {
            case SND_PCM_STREAM_PLAYBACK:
#endif
                if (!pcm_play_dma_complete_callback(PCM_DMAST_OK, &pcm_data, &pcm_size))
                {
                    return false;
                }
#ifdef HAVE_RECORDING
                break;
            case SND_PCM_STREAM_CAPTURE:
                if (!pcm_play_dma_complete_callback(PCM_DMAST_OK, &pcm_data, &pcm_size))
                {
                    return false;
                }
                break;
            default:
                break;
            }
#endif
        }

        /* Note:  This assumes stereo 16-bit */
        if (pcm_size % 4)
            panicf("Wrong pcm_size");
        /* the compiler will optimize this test away */
        nframes = MIN((ssize_t)pcm_size/4, frames_left);

#ifdef HAVE_RECORDING
        switch (current_alsa_mode)
        {
        case SND_PCM_STREAM_PLAYBACK:
#endif
#if defined(HAVE_ALSA_32BIT)
            if (pcm_playback_format() == SND_PCM_FORMAT_S32_LE)
            {
                /* We have to convert 16-bit to 32-bit, the need to multiply the
                 * sample by some value so the sound is not too low */
                const int16_t *pcm_ptr = pcm_data;
                sample_t *sample_ptr = &frames[2*(period_size-frames_left)];
                for (int i = 0; i < nframes; i++)
                {
                    *sample_ptr++ = (*pcm_ptr++ * dig_vol_mult_l) + PCM_DC_OFFSET_VALUE;
                    *sample_ptr++ = (*pcm_ptr++ * dig_vol_mult_r) + PCM_DC_OFFSET_VALUE;
                }
            }
            else if (pcm_playback_uses_bt())
            {
                const int16_t *pcm_ptr = pcm_data;
                int16_t *frame_ptr = (int16_t *)frames;
                int16_t *out = &frame_ptr[2 * (period_size - frames_left)];
                int i;

                for (i = 0; i < nframes; i++) {
                    int32_t sl = *pcm_ptr++ * dig_vol_mult_l;
                    int32_t sr = *pcm_ptr++ * dig_vol_mult_r;
                    *out++ = (int16_t)(sl >> 16);
                    *out++ = (int16_t)(sr >> 16);
                }
            }
            else
#endif
            {
                int16_t *frame_ptr = (int16_t *)frames;
                memcpy(&frame_ptr[2 * (period_size - frames_left)],
                       pcm_data, nframes * 4);
	    }
#ifdef HAVE_RECORDING
            break;
        case SND_PCM_STREAM_CAPTURE:
            memcpy(pcm_data_rec, &frames[2*(period_size-frames_left)], nframes * 4);
            break;
        default:
            break;
        }
#endif
        pcm_data += nframes*4;
        pcm_size -= nframes*4;
        frames_left -= nframes;

        if (new_buffer && !first)
        {
            new_buffer = false;
#ifdef HAVE_RECORDING
            switch (current_alsa_mode)
            {
            case SND_PCM_STREAM_PLAYBACK:
#endif
                pcm_play_dma_status_callback(PCM_DMAST_STARTED);
#ifdef HAVE_RECORDING
                break;
            case SND_PCM_STREAM_CAPTURE:
                pcm_rec_dma_status_callback(PCM_DMAST_STARTED);
                break;
            default:
                break;
            }
#endif
        }
    }

    return true;
}

static void pcm_poll_thread_stop(void);

/* Playback pump shared by SIGIO async handler and poll-thread fallback. */
static void pcm_pump_playback(snd_pcm_t *pcm)
{
    int err;
    snd_pcm_state_t state = snd_pcm_state(pcm);

    if (state == SND_PCM_STATE_XRUN)
    {
        xruns++;
        logf("initial underrun!");
        err = snd_pcm_recover(pcm, -EPIPE, 0);
        if (err < 0) {
            logf("XRUN Recovery error: %s", snd_strerror(err));
            return;
        }
    }
    else if (state == SND_PCM_STATE_DRAINING || state == SND_PCM_STATE_SETUP)
        return;

#ifdef HAVE_RECORDING
    if (current_alsa_mode != SND_PCM_STREAM_PLAYBACK)
        return;
#endif

    while (snd_pcm_avail_update(pcm) >= period_size)
    {
        if (copy_frames(false))
        {
        retry:
            err = snd_pcm_writei(pcm, frames, period_size);
            if (err == -EPIPE)
            {
                logf("mid underrun!");
                xruns++;
                err = snd_pcm_recover(pcm, -EPIPE, 0);
                if (err < 0) {
                    logf("XRUN Recovery error: %s", snd_strerror(err));
                    return;
                }
                goto retry;
            }
            else if (err != period_size)
            {
                logf("Write error: written %i expected %li", err, period_size);
                break;
            }
        }
        else
        {
            logf("%s: No Data (%d).", __func__, state);
            break;
        }
    }

    if (snd_pcm_state(pcm) == SND_PCM_STATE_PREPARED)
    {
        err = snd_pcm_start(pcm);
        if (err < 0)
            logf("cb start error: %s", snd_strerror(err));
    }
}

static void pcm_pump_capture(snd_pcm_t *pcm)
{
#ifdef HAVE_RECORDING
    int err;

    while (snd_pcm_avail_update(pcm) >= period_size)
    {
        err = snd_pcm_readi(pcm, frames, period_size);
        if (err == -EPIPE)
        {
            logf("rec mid underrun!");
            xruns++;
            err = snd_pcm_recover(pcm, -EPIPE, 0);
            if (err < 0) {
                logf("XRUN Recovery error: %s", snd_strerror(err));
                return;
            }
            continue;
        }
        else if (err != period_size)
        {
            logf("Read error: read %i expected %li", err, period_size);
            break;
        }

        if (!copy_frames(false))
            break;
    }
#else
    (void)pcm;
#endif
}

static void *pcm_poll_thread_fn(void *arg)
{
    (void)arg;

    while (pcm_poll_run) {
        sink_lock();
        if (handle && pcm_poll_playback) {
            snd_pcm_wait(handle, pcm_playback_uses_bt() ? 2 : 50);
#ifdef HAVE_RECORDING
            if (current_alsa_mode == SND_PCM_STREAM_PLAYBACK)
#endif
                pcm_pump_playback(handle);
#ifdef HAVE_RECORDING
            else if (current_alsa_mode == SND_PCM_STREAM_CAPTURE)
                pcm_pump_capture(handle);
#endif
        }
        sink_unlock();
    }
    return NULL;
}

static void pcm_poll_thread_start(void)
{
    if (pcm_poll_tid_valid)
        return;
    pcm_poll_run = true;
    if (pthread_create(&pcm_poll_tid, NULL, pcm_poll_thread_fn, NULL) == 0)
        pcm_poll_tid_valid = true;
    else
        pcm_poll_run = false;
}

static void pcm_poll_thread_stop(void)
{
    if (!pcm_poll_tid_valid)
        return;
    pcm_poll_run = false;
    pthread_join(pcm_poll_tid, NULL);
    pcm_poll_tid_valid = false;
}

static void async_callback(snd_async_handler_t *ahandler)
{
    snd_pcm_t *pcm;

    if (!ahandler)
        return;

    pcm = snd_async_handler_get_pcm(ahandler);
    if (!pcm)
        return;

    if (pthread_mutex_trylock(&pcm_mtx) != 0)
        return;

#ifdef HAVE_RECORDING
    if (current_alsa_mode == SND_PCM_STREAM_PLAYBACK)
#endif
        pcm_pump_playback(pcm);
#ifdef HAVE_RECORDING
    else if (current_alsa_mode == SND_PCM_STREAM_CAPTURE)
        pcm_pump_capture(pcm);
#endif

    pthread_mutex_unlock(&pcm_mtx);
}

static void close_hwdev(void)
{
    logf("closedev (%p)", handle);

    /* Stop poll thread before close; signal first so snd_pcm_wait can exit. */
    pcm_poll_run = false;

    if (handle) {
        /* snd_pcm_drain() can block forever on ALSA bluetooth PCM. */
        snd_pcm_drop(handle);
#ifdef AUDIOHW_MUTE_ON_STOP
        audiohw_mute(true);
#endif
        if (ahandler) {
            snd_async_del_handler(ahandler);
            ahandler = NULL;
        }
        snd_pcm_close(handle);
        handle = NULL;
    }

    pcm_poll_thread_stop();
    pcm_poll_playback = false;
    current_alsa_device = NULL;

#ifdef HAVE_RECORDING
    pcm_data_rec = NULL;
#endif
}

static void alsadev_cleanup(void)
{
    free(frames);
    frames = NULL;
    close_hwdev();
}

static void open_hwdev(const char *device, snd_pcm_stream_t mode)
{
    int err;

    logf("opendev %s (%p)", device, handle);

    if (handle && current_alsa_device && device
        && !strcmp(device, current_alsa_device)
#ifdef HAVE_RECORDING
        && current_alsa_mode == mode
#endif
        )
    {
        return;
    }

    /* Close old handle */
    close_hwdev();

    if ((err = snd_pcm_open(&handle, device, mode, 0)) < 0)
    {
        panicf("%s(): Cannot open device %s: %s", __func__, device, snd_strerror(err));
    }
    last_sample_rate = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&pcm_mtx, &attr);

    /* assign alternative stack for the signal handlers */
    stack_t ss = {
        .ss_sp = signal_stack,
        .ss_size = sizeof(signal_stack),
        .ss_flags = 0
    };
    struct sigaction sa;

    err = sigaltstack(&ss, NULL);
    if (err < 0)
    {
        panicf("Unable to install alternative signal stack: %s", strerror(err));
    }

    pcm_poll_playback = false;
    err = snd_async_add_pcm_handler(&ahandler, handle, async_callback, NULL);
    if (err < 0)
    {
        if (err == -ENOSYS && mode == SND_PCM_STREAM_PLAYBACK)
        {
            logf("PCM async unavailable, using poll thread: %s",
                 snd_strerror(err));
            pcm_poll_playback = true;
            pcm_poll_thread_start();
        }
        else
        {
            panicf("Unable to register async handler: %s", snd_strerror(err));
        }
    }
    else
    {
        /* only modify the stack the handler runs on */
        sigaction(SIGIO, NULL, &sa);
        sa.sa_flags |= SA_ONSTACK;
        err = sigaction(SIGIO, &sa, NULL);
        if (err < 0)
        {
            panicf("Unable to install alternative signal stack: %s", strerror(err));
        }
    }

#ifdef HAVE_RECORDING
    current_alsa_mode = mode;
#else
    (void)mode;
#endif
    current_alsa_device = device;

    atexit(alsadev_cleanup);
}

static void sink_dma_init(void)
{
    logf("PCM DMA Init");

    audiohw_preinit();

    open_hwdev(playback_dev, SND_PCM_STREAM_PLAYBACK);

    return;
}

static void sink_lock(void)
{
    pthread_mutex_lock(&pcm_mtx);
}

static void sink_unlock(void)
{
    pthread_mutex_unlock(&pcm_mtx);
}

bool pcm_alsa_reopen_playback_safe(void)
{
    snd_pcm_t *probe = NULL;
    bool ok;

    if (!playback_dev)
        return false;

    if (snd_pcm_open(&probe, playback_dev, SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return false;
    snd_pcm_close(probe);

    sink_lock();
    open_hwdev(playback_dev, SND_PCM_STREAM_PLAYBACK);
    sink_unlock();
    ok = (handle != NULL);
    return ok;
}

void pcm_alsa_reopen_playback(void)
{
    (void)pcm_alsa_reopen_playback_safe();
}

void pcm_alsa_release_playback_for_mixer(void)
{
    sink_lock();
    pcm_poll_run = false;
    if (handle) {
        snd_pcm_drop(handle);
        if (ahandler) {
            snd_async_del_handler(ahandler);
            ahandler = NULL;
        }
        snd_pcm_close(handle);
        handle = NULL;
        current_alsa_device = NULL;
    }
    pcm_poll_thread_stop();
    pcm_poll_playback = false;
    sink_unlock();
}

static void sink_set_freq_nolock(uint16_t freq)
{
    unsigned int sampr = hw_freq_sampr[freq];

    /* ALSA bluetooth plugin is S16 @ 44100 only on this platform. */
    if (pcm_playback_uses_bt())
        sampr = SAMPR_44;

    logf("PCM DMA Settings %d %lu", last_sample_rate, sampr);

    if (last_sample_rate != sampr)
    {
        last_sample_rate = sampr;

#ifdef AUDIOHW_MUTE_ON_SRATE_CHANGE
        audiohw_mute(true);
#endif
        snd_pcm_drop(handle);

        if (set_hwparams(handle, sampr) < 0)
            logf("set_hwparams failed %lu", sampr);
        else if (set_swparams(handle) < 0)
            logf("set_swparams failed");

#if defined(HAVE_NWZ_LINUX_CODEC)
        /* Sony NWZ linux driver uses a nonstandard mecanism to set the sampling rate */
        audiohw_set_frequency(sampr);
#endif
        /* (Will be unmuted by pcm resuming) */
    }
}

static void sink_set_freq(uint16_t freq)
{
    sink_lock();
    sink_set_freq_nolock(freq);
    sink_unlock();
}

void pcm_alsa_reconfigure_playback(void)
{
    uint16_t freq_idx = HW_FREQ_DEFAULT;

    sink_lock();
    last_sample_rate = 0;
    if (!handle) {
        sink_unlock();
        return;
    }
    if (pcm_playback_uses_bt())
        freq_idx = HW_FREQ_44;

    sink_set_freq_nolock(freq_idx);
    sink_unlock();
}

static void sink_dma_stop(void)
{
    logf("PCM DMA stop (%d)", snd_pcm_state(handle));

    if (handle) {
        int err = pcm_poll_playback ? snd_pcm_drop(handle) : snd_pcm_drain(handle);
        if (err < 0)
            logf("Stop failed: %s", snd_strerror(err));
    }
#ifdef AUDIOHW_MUTE_ON_STOP
    audiohw_mute(true);
#endif
}

static void sink_dma_start(const void *addr, size_t size)
{
    logf("PCM DMA start (%p %d)", addr, size);

    pcm_data = addr;
    pcm_size = size;

#if !defined(AUDIOHW_MUTE_ON_STOP) && defined(AUDIOHW_MUTE_ON_SRATE_CHANGE)
    audiohw_mute(false);
#endif

    while (1)
    {
        snd_pcm_state_t state = snd_pcm_state(handle);
        logf("PCM State %d", state);

        switch (state)
        {
            case SND_PCM_STATE_RUNNING:
#if defined(AUDIOHW_MUTE_ON_STOP)
                audiohw_mute(false);
#endif
#if defined(HAVE_EROSQ_LINUX_CODEC)
                erosq_notify_pcm_running();
#endif
                return;
            case SND_PCM_STATE_XRUN:
            {
                logf("Trying to recover from underrun");
                int err = snd_pcm_recover(handle, -EPIPE, 0);
                if (err < 0)
                    logf("Recovery failed: %s", snd_strerror(err));
                continue;
            }
            case SND_PCM_STATE_SETUP:
            {
                int err = snd_pcm_prepare(handle);
                if (err < 0)
                    logf("Prepare error: %s", snd_strerror(err));
            }
                /* fall through */
            case SND_PCM_STATE_PREPARED:
            {
                int err;
#if 0
                /* fill buffer with silence to initiate playback without noisy click */
                snd_pcm_sframes_t sample_size = buffer_size;
                sample_t *samples = calloc(1, sample_size * channels * sizeof(sample_t));

                snd_pcm_format_set_silence(format, samples, sample_size);
                err = snd_pcm_writei(handle, samples, sample_size);
                free(samples);

                if (err != (ssize_t)sample_size)
                {
                    logf("Initial write error: written %i expected %li", err, sample_size);
                    return;
                }
#else
                /* Fill buffer with proper sample data */
                {
                    snd_pcm_sframes_t prefilled = 0;
                    snd_pcm_sframes_t prefill_limit = pcm_playback_uses_bt()
                        ? period_size : buffer_size;

                    while (snd_pcm_avail_update(handle) >= period_size
                           && prefilled < prefill_limit)
                    {
                        if (copy_frames(true))
                        {
                            err = snd_pcm_writei(handle, frames, period_size);
                            if (err < 0 && err != period_size && err != -EAGAIN)
                            {
                                logf("Write error: written %i expected %li", err, period_size);
                                break;
                            }
                            prefilled += period_size;
                        }
                        else
                            break;
                    }
                }
#endif
                err = snd_pcm_start(handle);
                if (err < 0) {
                    logf("start error: %s", snd_strerror(err));
                    /* We will recover on the next iteration */
                }

                break;
            }
            case SND_PCM_STATE_DRAINING:
                /* run until drained */
                continue;
            default:
                logf("Unhandled state: %s", snd_pcm_state_name(state));
                return;
        }
    }
}

static void sink_dma_postinit(void)
{
    audiohw_postinit();

#ifdef AUDIOHW_NEEDS_INITIAL_UNMUTE
    audiohw_mute(false);
#endif
}

unsigned int pcm_alsa_get_rate(void)
{
    return real_sample_rate;
}

unsigned int pcm_alsa_get_xruns(void)
{
    return xruns;
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

#ifdef HAVE_RECORDING
void pcm_rec_lock(void)
{
    sink_lock();
}

void pcm_rec_unlock(void)
{
    sink_unlock();
}

void pcm_rec_dma_init(void)
{
    logf("PCM REC DMA Init");

    open_hwdev(capture_dev, SND_PCM_STREAM_CAPTURE);
}

void pcm_rec_dma_close(void)
{
    logf("Rec DMA Close");
    // close_hwdev();
    open_hwdev(playback_dev, SND_PCM_STREAM_PLAYBACK);
}

void pcm_rec_dma_start(void *start, size_t size)
{
    logf("PCM REC DMA start (%p %d)", start, size);
    pcm_data_rec = start;
    pcm_size = size;

    if (!handle) return;

    while (1)
    {
        snd_pcm_state_t state = snd_pcm_state(handle);

        switch (state)
        {
            case SND_PCM_STATE_RUNNING:
                return;
            case SND_PCM_STATE_XRUN:
            {
                logf("Trying to recover from error");
                int err = snd_pcm_recover(handle, -EPIPE, 0);
                if (err < 0)
                    panicf("Recovery failed: %s", snd_strerror(err));
                continue;
            }
            case SND_PCM_STATE_SETUP:
            {
                int err = snd_pcm_prepare(handle);
                if (err < 0)
                    panicf("Prepare error: %s", snd_strerror(err));
            }
                /* fall through */
            case SND_PCM_STATE_PREPARED:
            {
                int err = snd_pcm_start(handle);
                if (err < 0)
                    panicf("Start error: %s", snd_strerror(err));
                return;
            }
            case SND_PCM_STATE_DRAINING:
                /* run until drained */
                continue;
            default:
                logf("Unhandled state: %s", snd_pcm_state_name(state));
                return;
        }
    }
}

void pcm_rec_dma_stop(void)
{
    logf("Rec DMA Stop");
    close_hwdev();
}

const void * pcm_rec_dma_get_peak_buffer(void)
{
    uintptr_t addr = (uintptr_t)pcm_data_rec;
    return (void*)((addr + 3) & ~3);
}

#ifdef SIMULATOR
void audiohw_set_recvol(int left, int right, int type)
{
    (void)left;
    (void)right;
    (void)type;
}
#endif

#endif /* HAVE_RECORDING */
