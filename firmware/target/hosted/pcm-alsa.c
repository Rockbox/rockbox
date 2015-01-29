/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
 * Copyright (C) 2014 by Udo Schläpfer
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

#include "audiohw.h"
#include "config.h"
#include "debug.h"
#include "panic.h"
#include "pcm.h"
#include "pcm-internal.h"
#include "tick.h"

#include "sound/asound.h"
#include "tinyalsa/asoundlib.h"


/* Tiny alsa handle. */
static struct pcm* _alsa_handle = NULL;


/* Bytes left in the Rockbox PCM frame buffer. */
static size_t _pcm_buffer_size = 0;


/* Rockbox PCM frame buffer. */
static const void* _pcm_buffer = NULL;


/* Size of the internal PCM frame buffer. */
static size_t _frame_buffer_size = 0;


/* Internal PCM frame buffer. */
static char* _frame_buffer = NULL;


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

        size_t frame_buffer_size_left = _frame_buffer_size;
        size_t frame_buffer_position  = 0;

        memset(_frame_buffer, 0, _frame_buffer_size);

        /* Fill the internal PCM buffer from the Rockbox PCM buffer. */
        while(frame_buffer_size_left > 0)
        {
            if(_pcm_buffer_size == 0)
            {
                /* Retrive a new PCM buffer from Rockbox. */
                if(! pcm_play_dma_complete_callback(PCM_DMAST_OK, &_pcm_buffer, &_pcm_buffer_size))
                {
                    DEBUGF("DEBUG %s: No new buffer.", __func__);
                    break;
                }
                pcm_play_dma_status_callback(PCM_DMAST_STARTED);
            }

            DEBUGF("DEBUG %s: _pcm_buffer_size: %d, frame_buffer_size_left: %d, frame_buffer_position: %d.", __func__, _pcm_buffer_size, frame_buffer_size_left, frame_buffer_position);

            /* Number of bytes to copy. */
            size_t copy_now = MIN(_pcm_buffer_size, frame_buffer_size_left);

            /* Copy Rockbox PCM buffer to internal PCM frame buffer. */
            memcpy(&_frame_buffer[frame_buffer_position], _pcm_buffer, copy_now);

            /* Adjust buffers. */
            _pcm_buffer            += copy_now;
            _pcm_buffer_size       -= copy_now;
            frame_buffer_position  += copy_now;
            frame_buffer_size_left -= copy_now;
        }

        if(pcm_write(_alsa_handle, _frame_buffer, _frame_buffer_size) != 0)
        {
            DEBUGF("ERROR %s: pcm_write failed: %s.", __func__, pcm_get_error(_alsa_handle));

            usleep( 10000 );
            continue;
        }

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
    audiohw_preinit();

#ifdef DEBUG

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

#endif

    if((_alsa_handle != NULL) || (_frame_buffer != NULL))
    {
        DEBUGF("ERROR %s: Allready initialized.", __func__);
        panicf("ERROR %s: Allready initialized.", __func__);
        return;
    }

    /*
        ToDo: This needs a interface to enable device specific config of ALSA.

        Rockbox outputs 16 Bit/44.1kHz stereo by default.

        ALSA frame buffer size = config.period_count * config.period_size * config.channels * (16 \ 8)
                               = 4 * 256 * 2 * 2
                               = 4096
    */
    _config.channels          = 2;
    _config.rate              = 44100;
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

    _frame_buffer_size = pcm_frames_to_bytes(_alsa_handle, pcm_get_buffer_size(_alsa_handle));

    DEBUGF("DEBUG %s: ALSA PCM frame buffer size: %d.", __func__, _frame_buffer_size);

    _frame_buffer = malloc(_frame_buffer_size);
    if(_frame_buffer == NULL)
    {
        DEBUGF("ERROR %s: malloc for _frame_buffer failed.", __func__);
        panicf("ERROR %s: malloc for _frame_buffer failed.", __func__);
        return;
    }

    /* Create pcm thread in the suspended state. */
    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = 1;
    _dma_locked  = 1;
    pthread_create(&_pcm_thread, NULL, pcm_thread_run, NULL);
    pthread_mutex_unlock(&_dma_suspended_mtx);
}


void pcm_play_dma_start(const void *addr, size_t size)
{
    _pcm_buffer      = addr;
    _pcm_buffer_size = size;

    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = 0;
    pthread_cond_signal(&_dma_suspended_cond);
    pthread_mutex_unlock(&_dma_suspended_mtx);
}


void pcm_play_dma_pause(bool pause)
{
    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = pause ? 1 : 0;
    if(_dma_stopped == 0)
    {
        pthread_cond_signal(&_dma_suspended_cond);
    }
    pthread_mutex_unlock(&_dma_suspended_mtx);
}


void pcm_play_dma_stop(void)
{
    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = 1;
    pcm_stop(_alsa_handle);
    pthread_mutex_unlock(&_dma_suspended_mtx);
}


/* Unessecary play locks before pcm_play_dma_postinit. */
static int _play_lock_recursion_count = -10000;


void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
    _play_lock_recursion_count = 0;
}


void pcm_play_lock(void)
{
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

    if(_config.rate != rate)
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


size_t pcm_get_bytes_waiting(void)
{
    return _pcm_buffer_size;
}


const void* pcm_play_dma_get_peak_buffer(int* count)
{
    uintptr_t addr = (uintptr_t) _pcm_buffer;
    *count = _pcm_buffer_size / 4;
    return (void*) ((addr + 3) & ~3);
}


void pcm_close_device(void)
{
    pthread_mutex_lock(&_dma_suspended_mtx);
    _dma_stopped = 1;
    pthread_mutex_unlock(&_dma_suspended_mtx);

    pcm_close(_alsa_handle);
    _alsa_handle = NULL;

    free(_frame_buffer);
    _frame_buffer = NULL;
}


#ifdef HAVE_RECORDING


void pcm_rec_lock(void)
{}


void pcm_rec_unlock(void)
{}


void pcm_rec_dma_init(void)
{}


void pcm_rec_dma_close(void)
{}


void pcm_rec_dma_start(void *start, size_t size)
{
    (void) start;
    (void) size;
}


void pcm_rec_dma_stop(void)
{}


const void* pcm_rec_dma_get_peak_buffer(void)
{
    return NULL;
}


void audiohw_set_recvol(int left, int right, int type)
{
    (void) left;
    (void) right;
    (void) type;
}


#endif /* HAVE_RECORDING */
