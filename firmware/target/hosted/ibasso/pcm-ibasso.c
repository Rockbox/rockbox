/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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


#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#include "config.h"
#include "debug.h"
#include "panic.h"
#include "pcm.h"
#include "pcm-internal.h"

#include "sound/asound.h"
#include "tinyalsa/asoundlib.h"

#include "debug-ibasso.h"
#include "sysfs-ibasso.h"


/* Tiny alsa handle. */
static struct pcm* _alsa_handle = NULL;


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
static volatile sig_atomic_t _dma_stopped = 1;
static volatile sig_atomic_t _dma_locked  = 1;


/* Mutex for PCM thread suspend/unsuspend. */
static pthread_mutex_t _dma_suspended_mtx = PTHREAD_MUTEX_INITIALIZER;


/* Signal condition for PCM thread suspend/unsuspend. */
static pthread_cond_t _dma_suspended_cond = PTHREAD_COND_INITIALIZER;


static void* pcm_thread_run(void* nothing)
{
    (void) nothing;

    DEBUGF("DEBUG %s: Thread start.", __func__);

    while(true)
    {
        pthread_mutex_lock(&_dma_suspended_mtx);
        while((_dma_stopped == 1) || (_dma_locked == 1))
        {
            DEBUGF("DEBUG %s: Playback suspended.", __func__);
            pthread_cond_wait(&_dma_suspended_cond, &_dma_suspended_mtx);
            DEBUGF("DEBUG %s: Playback resumed.", __func__);
        }
        pthread_mutex_unlock(&_dma_suspended_mtx);

        if(_pcm_buffer_size == 0)
        {
            /* Retrive a new PCM buffer from Rockbox. */
            if(! pcm_play_dma_complete_callback(PCM_DMAST_OK, &_pcm_buffer, &_pcm_buffer_size))
            {
                DEBUGF("DEBUG %s: No new buffer.", __func__);

                usleep( 10000 );
                continue;
            }
        }
        pcm_play_dma_status_callback(PCM_DMAST_STARTED);

        /* This relies on Rockbox PCM frame buffer size == ALSA PCM frame buffer size. */
        if(pcm_write(_alsa_handle, _pcm_buffer, _pcm_buffer_size) != 0)
        {
            DEBUGF("ERROR %s: pcm_write failed: %s.", __func__, pcm_get_error(_alsa_handle));

            usleep( 10000 );
            continue;
        }

        _pcm_buffer_size = 0;

        /*DEBUGF("DEBUG %s: Thread running.", __func__);*/
    }

    DEBUGF("DEBUG %s: Thread end.", __func__);

    return 0;
}


#ifdef DEBUG

/* https://github.com/tinyalsa/tinyalsa/blob/master/tinypcminfo.c */

static const char* format_lookup[] =
{
    /*[0] =*/ "S8",
    "U8",
    "S16_LE",
    "S16_BE",
    "U16_LE",
    "U16_BE",
    "S24_LE",
    "S24_BE",
    "U24_LE",
    "U24_BE",
    "S32_LE",
    "S32_BE",
    "U32_LE",
    "U32_BE",
    "FLOAT_LE",
    "FLOAT_BE",
    "FLOAT64_LE",
    "FLOAT64_BE",
    "IEC958_SUBFRAME_LE",
    "IEC958_SUBFRAME_BE",
    "MU_LAW",
    "A_LAW",
    "IMA_ADPCM",
    "MPEG",
    /*[24] =*/ "GSM",
    [31] = "SPECIAL",
    "S24_3LE",
    "S24_3BE",
    "U24_3LE",
    "U24_3BE",
    "S20_3LE",
    "S20_3BE",
    "U20_3LE",
    "U20_3BE",
    "S18_3LE",
    "S18_3BE",
    "U18_3LE",
    /*[43] =*/ "U18_3BE"
};


static const char* pcm_get_format_name(unsigned int bit_index)
{
    return(bit_index < 43 ? format_lookup[bit_index] : NULL);
}

#endif


/* Thread that copies the Rockbox PCM buffer to ALSA. */
static pthread_t _pcm_thread;


/* ALSA card and device. */
static const unsigned int CARD   = 0;
static const unsigned int DEVICE = 0;


/* ALSA config. */
static struct pcm_config _config;


void pcm_play_dma_init(void)
{
    TRACE;

#ifdef DEBUG

    /*
        DEBUG pcm_play_dma_init: Access: 0x000009
        DEBUG pcm_play_dma_init: Format[0]: 0x000044
        DEBUG pcm_play_dma_init: Format[1]: 0x000010
        DEBUG pcm_play_dma_init: Format: S16_LE
        DEBUG pcm_play_dma_init: Format: S24_LE
        DEBUG pcm_play_dma_init: Format: S20_3LE
        DEBUG pcm_play_dma_init: Subformat: 0x000001
        DEBUG pcm_play_dma_init: Rate: min = 8000Hz, max = 192000Hz
        DEBUG pcm_play_dma_init: Channels: min = 2, max = 2
        DEBUG pcm_play_dma_init: Sample bits: min=16, max=32
        DEBUG pcm_play_dma_init: Period size: min=8, max=10922
        DEBUG pcm_play_dma_init: Period count: min=3, max=128
        DEBUG pcm_play_dma_init: 0 mixer controls.
    */

    struct pcm_params* params = pcm_params_get(CARD, DEVICE, PCM_OUT);
    if(params == NULL)
    {
        DEBUGF("ERROR %s: Card/device does not exist.", __func__);
        panicf("ERROR %s: Card/device does not exist.", __func__);
        return;
    }

    struct pcm_mask* m = pcm_params_get_mask(params, PCM_PARAM_ACCESS);
    if(m)
    {
        DEBUGF("DEBUG %s: Access: %#08x", __func__, m->bits[0]);
    }

    m = pcm_params_get_mask(params, PCM_PARAM_FORMAT);
    if(m)
    {
        DEBUGF("DEBUG %s: Format[0]: %#08x", __func__, m->bits[0]);
        DEBUGF("DEBUG %s: Format[1]: %#08x", __func__, m->bits[1]);

        unsigned int j;
        unsigned int k;
        const unsigned int bitcount = sizeof(m->bits[0]) * 8;
        for(k = 0; k < 2; ++k)
        {
            for(j = 0; j < bitcount; ++j)
            {
                const char* name;
                if(m->bits[k] & (1 << j))
                {
                    name = pcm_get_format_name(j + (k * bitcount));
                    if(name)
                    {
                        DEBUGF("DEBUG %s: Format: %s", __func__, name);
                    }
                }
            }
        }
    }

    m = pcm_params_get_mask(params, PCM_PARAM_SUBFORMAT);
    if(m)
    {
        DEBUGF("DEBUG %s: Subformat: %#08x", __func__, m->bits[0]);
    }

    unsigned int min = pcm_params_get_min(params, PCM_PARAM_RATE);
    unsigned int max = pcm_params_get_max(params, PCM_PARAM_RATE) ;
    DEBUGF("DEBUG %s: Rate: min = %uHz, max = %uHz", __func__, min, max);

    min = pcm_params_get_min(params, PCM_PARAM_CHANNELS);
    max = pcm_params_get_max(params, PCM_PARAM_CHANNELS);
    DEBUGF("DEBUG %s: Channels: min = %u, max = %u", __func__, min, max);

    min = pcm_params_get_min(params, PCM_PARAM_SAMPLE_BITS);
    max = pcm_params_get_max(params, PCM_PARAM_SAMPLE_BITS);
    DEBUGF("DEBUG %s: Sample bits: min=%u, max=%u", __func__, min, max);

    min = pcm_params_get_min(params, PCM_PARAM_PERIOD_SIZE);
    max = pcm_params_get_max(params, PCM_PARAM_PERIOD_SIZE);
    DEBUGF("DEBUG %s: Period size: min=%u, max=%u", __func__, min, max);

    min = pcm_params_get_min(params, PCM_PARAM_PERIODS);
    max = pcm_params_get_max(params, PCM_PARAM_PERIODS);
    DEBUGF("DEBUG %s: Period count: min=%u, max=%u", __func__, min, max);

    pcm_params_free(params);

    struct mixer* mixer = mixer_open(CARD);
    if(! mixer)
    {
        DEBUGF("ERROR %s: Failed to open mixer.", __func__);
    }
    else
    {
        int num_ctls = mixer_get_num_ctls(mixer);

        DEBUGF("DEBUG %s: %d mixer controls.", __func__, num_ctls);

        mixer_close(mixer);
    }

#endif

    if(_alsa_handle != NULL)
    {
        DEBUGF("ERROR %s: Allready initialized.", __func__);
        panicf("ERROR %s: Allready initialized.", __func__);
        return;
    }

    /*
        Rockbox outputs 16 Bit/44.1kHz stereo by default.

        ALSA frame buffer size = config.period_count * config.period_size * config.channels * (16 \ 8)
                               = 4 * 256 * 2 * 2
                               = 4096
                               = Rockbox PCM buffer size
        pcm_thread_run relies on this size match. See pcm_mixer.h.
    */
    _config.channels          = 2;
    _config.rate              = pcm_sampr;
    _config.period_size       = 256;
    _config.period_count      = 4;
    _config.format            = PCM_FORMAT_S16_LE;
    _config.start_threshold   = 0;
    _config.stop_threshold    = 0;
    _config.silence_threshold = 0;

    _alsa_handle = pcm_open(CARD, DEVICE, PCM_OUT, &_config);
    if(! pcm_is_ready(_alsa_handle))
    {
        DEBUGF("ERROR %s: pcm_open failed: %s.", __func__, pcm_get_error(_alsa_handle));
        panicf("ERROR %s: pcm_open failed: %s.", __func__, pcm_get_error(_alsa_handle));
        return;
    }

    DEBUGF("DEBUG %s: ALSA PCM frame buffer size: %d.", __func__, pcm_frames_to_bytes(_alsa_handle, pcm_get_buffer_size(_alsa_handle)));

    /* Create pcm thread in the suspended state. */
    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = 1;
    _dma_locked  = 1;
    pthread_create(&_pcm_thread, NULL, pcm_thread_run, NULL);
    pthread_mutex_unlock(&_dma_suspended_mtx);
}


void pcm_play_dma_start(const void *addr, size_t size)
{
    TRACE;

    /*
        DX50
        /sys/class/codec/mute
        Mute:   echo 'A' > /sys/class/codec/mute
        Unmute: echo 'B' > /sys/class/codec/mute

        DX90?
    */
    if(! sysfs_set_char(SYSFS_MUTE, 'B'))
    {
        DEBUGF("ERROR %s: Could not unmute.", __func__);
        panicf("ERROR %s: Could not unmute.", __func__);
    }

    _pcm_buffer      = addr;
    _pcm_buffer_size = size;

    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = 0;
    pthread_cond_signal(&_dma_suspended_cond);
    pthread_mutex_unlock(&_dma_suspended_mtx);
}

void pcm_play_dma_stop(void)
{
    TRACE;

    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = 1;
    pcm_stop(_alsa_handle);
    pthread_mutex_unlock(&_dma_suspended_mtx);
}


/* Unessecary play locks before pcm_play_dma_postinit. */
static int _play_lock_recursion_count = -10000;


void pcm_play_dma_postinit(void)
{
    TRACE;

    _play_lock_recursion_count = 0;
}


void pcm_play_lock(void)
{
    TRACE;

    ++_play_lock_recursion_count;

    if(_play_lock_recursion_count == 1)
    {
        pthread_mutex_lock(&_dma_suspended_mtx);
        _dma_locked = 1;
        pthread_mutex_unlock(&_dma_suspended_mtx);
    }
}


void pcm_play_unlock(void)
{
    TRACE;

    --_play_lock_recursion_count;

    if(_play_lock_recursion_count == 0)
    {
        pthread_mutex_lock(&_dma_suspended_mtx);
        _dma_locked = 0;
        pthread_cond_signal(&_dma_suspended_cond);
        pthread_mutex_unlock(&_dma_suspended_mtx);
    }
}


void pcm_dma_apply_settings(void)
{
    unsigned int rate = pcm_get_frequency();

    DEBUGF("DEBUG %s: Current sample rate: %u, next sampe rate: %u.", __func__, _config.rate, rate);

    if(( _config.rate != rate) && (rate >= 8000) && (rate <= 192000))
    {
        _config.rate = rate;

        pcm_close(_alsa_handle);
        _alsa_handle = pcm_open(CARD, DEVICE, PCM_OUT, &_config);

        if(! pcm_is_ready(_alsa_handle))
        {
            DEBUGF("ERROR %s: pcm_open failed: %s.", __func__, pcm_get_error(_alsa_handle));
            panicf("ERROR %s: pcm_open failed: %s.", __func__, pcm_get_error(_alsa_handle));
        }
    }
}

/* TODO: WTF */
const void* pcm_play_dma_get_peak_buffer(int* count)
{
    TRACE;

    uintptr_t addr = (uintptr_t) _pcm_buffer;
    *count = _pcm_buffer_size / 4;
    return (void*) ((addr + 3) & ~3);
}


void pcm_close_device(void)
{
    TRACE;

    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = 1;
    pthread_mutex_unlock(&_dma_suspended_mtx);

    pcm_close(_alsa_handle);
    _alsa_handle = NULL;
}
