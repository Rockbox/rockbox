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
#include "audiohw.h"
#include "pcm-alsa.h"

#include "logf.h"

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

static const snd_pcm_access_t access_ = SND_PCM_ACCESS_RW_INTERLEAVED; /* access mode */
#if defined(SONY_NWZ_LINUX) || defined(HAVE_FIIO_LINUX_CODEC)
/* Sony NWZ must use 32-bit per sample */
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
static char signal_stack[SIGSTKSZ];

static const char *playback_dev = DEFAULT_PLAYBACK_DEVICE;

#ifdef HAVE_RECORDING
static void *pcm_data_rec = DEFAULT_CAPTURE_DEVICE;
static const char *capture_dev = NULL;
static snd_pcm_stream_t current_alsa_mode;  /* SND_PCM_STREAM_PLAYBACK / _CAPTURE */
#endif

static const char *current_alsa_device;

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

static int set_hwparams(snd_pcm_t *handle)
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
    if (pcm_sampr > SAMPR_96) {
        buffer_size = MIX_FRAME_SAMPLES * 4 * 4;
        period_size = MIX_FRAME_SAMPLES * 4;
    } else if (pcm_sampr > SAMPR_48) {
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
    err = snd_pcm_hw_params_set_format(handle, params, format);
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
    srate = pcm_sampr;
    err = snd_pcm_hw_params_set_rate_near(handle, params, &srate, 0);
    if (err < 0)
    {
        logf("Rate %luHz not available for playback: %s", pcm_sampr, snd_strerror(err));
        goto error;
    }
    real_sample_rate = srate;
    if (real_sample_rate != pcm_sampr)
    {
        logf("Rate doesn't match (requested %luHz, get %dHz)", pcm_sampr, real_sample_rate);
        err = -EINVAL;
        goto error;
    }

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
    frames = calloc(1, period_size * channels * sizeof(sample_t));

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
    /* start the transfer when the buffer is half full */
    err = snd_pcm_sw_params_set_start_threshold(handle, swparams, buffer_size / 2);
    if (err < 0)
    {
        logf("Unable to set start threshold mode for playback: %s", snd_strerror(err));
        goto error;
    }
    /* allow the transfer when at least period_size samples can be processed */
    err = snd_pcm_sw_params_set_avail_min(handle, swparams, period_size);
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

/* Digital volume explanation:
 * with very good approximation (<0.1dB) the convertion from dB to multiplicative
 * factor, for dB>=0, is 2^(dB/3). We can then notice that if we write dB=3*k+r
 * then this is 2^k*2^(r/3) so we only need to look at r=0,1,2. For r=0 this is
 * 1, for r=1 we have 2^(1/3)~=1.25 so we approximate by 1+1/4, and 2^(2/3)~=1.5
 * so we approximate by 1+1/2. To go from negative to nonnegative we notice that
 * 48 dB => 63095 factor ~= 2^16 so we virtually pre-multiply everything by 2^(-16)
 * and add 48dB to the input volume. We cannot go lower -43dB because several
 * values between -48dB and -43dB would require a fractional multiplier, which is
 * stupid to implement for such very low volume. */
static int dig_vol_mult_l = 2 << 16; /* multiplicative factor to apply to each sample */
static int dig_vol_mult_r = 2 << 16; /* multiplicative factor to apply to each sample */
void pcm_set_mixer_volume(int vol_db_l, int vol_db_r)
{
    if(vol_db_l > 0 || vol_db_r > 0 || vol_db_l < -43 || vol_db_r < -43)
        panicf("invalid pcm alsa volume");
    if(format != SND_PCM_FORMAT_S32_LE)
        panicf("this function assumes 32-bit sample size");
    vol_db_l += 48; /* -42dB .. 0dB => 5dB .. 48dB */
    vol_db_r += 48; /* -42dB .. 0dB => 5dB .. 48dB */
    /* NOTE if vol_dB = 5 then vol_shift = 1 but r = 1 so we do vol_shift - 1 >= 0
     * otherwise vol_dB >= 0 implies vol_shift >= 2 so vol_shift - 2 >= 0 */
    int vol_shift_l = vol_db_l / 3;
    int vol_shift_r = vol_db_r / 3;
    int r_l = vol_db_l % 3;
    int r_r = vol_db_r % 3;
    if(r_l == 0)
        dig_vol_mult_l = 1 << vol_shift_l;
    else if(r_l == 1)
        dig_vol_mult_l = 1 << vol_shift_l | 1 << (vol_shift_l - 2);
    else
        dig_vol_mult_l = 1 << vol_shift_l | 1 << (vol_shift_l - 1);
    logf("l: %d dB -> factor = %d", vol_db_l - 48, dig_vol_mult_l);
    if(r_r == 0)
        dig_vol_mult_r = 1 << vol_shift_r;
    else if(r_r == 1)
        dig_vol_mult_r = 1 << vol_shift_r | 1 << (vol_shift_r - 2);
    else
        dig_vol_mult_r = 1 << vol_shift_r | 1 << (vol_shift_r - 1);
    logf("r: %d dB -> factor = %d", vol_db_r - 48, dig_vol_mult_r);
}

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
        if (format == SND_PCM_FORMAT_S32_LE)
        {
            /* We have to convert 16-bit to 32-bit, the need to multiply the
             * sample by some value so the sound is not too low */
            const int16_t *pcm_ptr = pcm_data;
            sample_t *sample_ptr = &frames[2*(period_size-frames_left)];
            for (int i = 0; i < nframes; i++)
            {
                *sample_ptr++ = *pcm_ptr++ * dig_vol_mult_l;
                *sample_ptr++ = *pcm_ptr++ * dig_vol_mult_r;
            }
        }
        else
        {
#ifdef HAVE_RECORDING
            switch (current_alsa_mode)
            {
            case SND_PCM_STREAM_PLAYBACK:
#endif
                /* Rockbox and PCM have same format: memcopy */
                memcpy(&frames[2*(period_size-frames_left)], pcm_data, nframes * 4);
#ifdef HAVE_RECORDING
                break;
            case SND_PCM_STREAM_CAPTURE:
                memcpy(pcm_data_rec, &frames[2*(period_size-frames_left)], nframes * 4);
                break;
            default:
                break;
            }
#endif
        }
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

static void async_callback(snd_async_handler_t *ahandler)
{
    int err;

    if (!ahandler) return;

    snd_pcm_t *handle = snd_async_handler_get_pcm(ahandler);

    if (!handle) return;

    if (pthread_mutex_trylock(&pcm_mtx) != 0)
        return;

    snd_pcm_state_t state = snd_pcm_state(handle);

    if (state == SND_PCM_STATE_XRUN)
    {
        xruns++;
        logf("initial underrun!");
        err = snd_pcm_recover(handle, -EPIPE, 0);
        if (err < 0) {
            logf("XRUN Recovery error: %s", snd_strerror(err));
            goto abort;
        }
    }
    else if (state == SND_PCM_STATE_DRAINING)
    {
        logf("draining...");
        goto abort;
    }
    else if (state == SND_PCM_STATE_SETUP)
    {
        goto abort;
    }

#ifdef HAVE_RECORDING
    if (current_alsa_mode == SND_PCM_STREAM_PLAYBACK)
    {
#endif
        while (snd_pcm_avail_update(handle) >= period_size)
        {
            if (copy_frames(false))
            {
            retry:
                err = snd_pcm_writei(handle, frames, period_size);
                if (err == -EPIPE)
                {
                    logf("mid underrun!");
                    xruns++;
                    err = snd_pcm_recover(handle, -EPIPE, 0);
                    if (err < 0) {
                       logf("XRUN Recovery error: %s", snd_strerror(err));
                       goto abort;
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
#ifdef HAVE_RECORDING
    }
    else if (current_alsa_mode == SND_PCM_STREAM_CAPTURE)
    {
        while (snd_pcm_avail_update(handle) >= period_size)
        {
            int err = snd_pcm_readi(handle, frames, period_size);
            if (err == -EPIPE)
            {
                logf("rec mid underrun!");
                xruns++;
                err = snd_pcm_recover(handle, -EPIPE, 0);
                if (err < 0) {
                   logf("XRUN Recovery error: %s", snd_strerror(err));
                   goto abort;
                }
		continue;  /* buffer contents trashed, no sense in trying to copy */
            }
            else if (err != period_size)
            {
                logf("Read error: read %i expected %li", err, period_size);
                break;
            }

            /* start the fake DMA transfer */
            if (!copy_frames(false))
            {
                /* do not spam logf */
                /* logf("%s: No Data.", __func__); */
                break;
            }
        }
    }
#endif

    if (snd_pcm_state(handle) == SND_PCM_STATE_PREPARED)
    {
        err = snd_pcm_start(handle);
        if (err < 0) {
            logf("cb start error: %s", snd_strerror(err));
            /* Depending on the error we might be SOL */
        }
    }

abort:
    pthread_mutex_unlock(&pcm_mtx);
}

static void close_hwdev(void)
{
    logf("closedev (%p)", handle);

    if (handle) {
        snd_pcm_drain(handle);
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

    if (handle && device == current_alsa_device
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

    err = snd_async_add_pcm_handler(&ahandler, handle, async_callback, NULL);
    if (err < 0)
    {
        panicf("Unable to register async handler: %s", snd_strerror(err));
    }

    /* only modify the stack the handler runs on */
    sigaction(SIGIO, NULL, &sa);
    sa.sa_flags |= SA_ONSTACK;
    err = sigaction(SIGIO, &sa, NULL);
    if (err < 0)
    {
        panicf("Unable to install alternative signal stack: %s", strerror(err));
    }

#ifdef HAVE_RECORDING
    current_alsa_mode = mode;
#else
    (void)mode;
#endif
    current_alsa_device = device;

    atexit(alsadev_cleanup);
}

void pcm_play_dma_init(void)
{
    logf("PCM DMA Init");

    audiohw_preinit();

    open_hwdev(playback_dev, SND_PCM_STREAM_PLAYBACK);

    return;
}

void pcm_play_lock(void)
{
    pthread_mutex_lock(&pcm_mtx);
}

void pcm_play_unlock(void)
{
    pthread_mutex_unlock(&pcm_mtx);
}

static void pcm_dma_apply_settings_nolock(void)
{
    logf("PCM DMA Settings %d %lu", last_sample_rate, pcm_sampr);

    if (last_sample_rate != pcm_sampr)
    {
        last_sample_rate = pcm_sampr;

#ifdef AUDIOHW_MUTE_ON_SRATE_CHANGE
        audiohw_mute(true);
#endif
        snd_pcm_drop(handle);

        set_hwparams(handle); // FIXME: check return code?
        set_swparams(handle); // FIXME: check return code?

#if defined(HAVE_NWZ_LINUX_CODEC)
        /* Sony NWZ linux driver uses a nonstandard mecanism to set the sampling rate */
        audiohw_set_frequency(pcm_sampr);
#endif
        /* (Will be unmuted by pcm resuming) */
    }
}

void pcm_dma_apply_settings(void)
{
    pcm_play_lock();
    pcm_dma_apply_settings_nolock();
    pcm_play_unlock();
}

void pcm_play_dma_stop(void)
{
    logf("PCM DMA stop (%d)", snd_pcm_state(handle));

    int err = snd_pcm_drain(handle);
    if (err < 0)
        if (err < 0)
            logf("Drain failed: %s", snd_strerror(err));
#ifdef AUDIOHW_MUTE_ON_STOP
    audiohw_mute(true);
#endif
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    logf("PCM DMA start (%p %d)", addr, size);
    pcm_dma_apply_settings_nolock();

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
                while (snd_pcm_avail_update(handle) >= period_size)
                {
                    if (copy_frames(true))
                    {
                        err = snd_pcm_writei(handle, frames, period_size);
                        if (err < 0 && err != period_size && err != -EAGAIN)
                        {
                            logf("Write error: written %i expected %li", err, period_size);
                            break;
                        }
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

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    uintptr_t addr = (uintptr_t)pcm_data;
    *count = pcm_size / 4;
    return (void *)((addr + 3) & ~3);
}

void pcm_play_dma_postinit(void)
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

#ifdef HAVE_RECORDING
void pcm_rec_lock(void)
{
    pcm_play_lock();
}

void pcm_rec_unlock(void)
{
    pcm_play_unlock();
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
    pcm_dma_apply_settings_nolock();
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
