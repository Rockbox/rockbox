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

#include "autoconf.h"

#include <stdlib.h>
#include <stdbool.h>
#include <SDL.h>
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

/*#define LOGF_ENABLE*/
#include "logf.h"

#ifdef DEBUG
#include <stdio.h>
extern bool debug_audio;
#endif

static int cvt_status = -1;

static const void *pcm_data;
static size_t pcm_data_size;
static size_t pcm_sample_bytes;
static size_t pcm_channel_bytes;

static struct pcm_udata
{
    Uint8 *stream;
    Uint32 num_in;
    Uint32 num_out;
#ifdef DEBUG
    FILE  *debug;
#endif
} udata;

static SDL_AudioSpec obtained;
static SDL_AudioCVT cvt;
static int audio_locked = 0;
static SDL_mutex *audio_lock;

void pcm_play_lock(void)
{
    if (++audio_locked == 1)
        SDL_LockMutex(audio_lock);
}

void pcm_play_unlock(void)
{
    if (--audio_locked == 0)
        SDL_UnlockMutex(audio_lock);
}

static void sdl_audio_callback(struct pcm_udata *udata, Uint8 *stream, int len);
static void pcm_dma_apply_settings_nolock(void)
{
    SDL_AudioSpec wanted_spec;
    wanted_spec.freq = pcm_sampr;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 2;
    wanted_spec.samples = 2048;
    wanted_spec.callback =
        (void (SDLCALL *)(void *userdata,
            Uint8 *stream, int len))sdl_audio_callback;
    wanted_spec.userdata = &udata;
    SDL_CloseAudio();
    /* Open the audio device and start playing sound! */
    if(SDL_OpenAudio(&wanted_spec, &obtained) < 0) {
        DEBUGF("Unable to open audio: %s\n", SDL_GetError());
        return;
    }
    switch (obtained.format)
    {
    case AUDIO_U8:
    case AUDIO_S8:
        pcm_channel_bytes = 1;
        break;
    case AUDIO_U16LSB:
    case AUDIO_S16LSB:
    case AUDIO_U16MSB:
    case AUDIO_S16MSB:
        pcm_channel_bytes = 2;
        break;
    default:
        DEBUGF("Unknown sample format obtained: %u\n",
                (unsigned)obtained.format);
        return;
    }
    pcm_sample_bytes = obtained.channels * pcm_channel_bytes;

    cvt_status = SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, 2, pcm_sampr,
                    obtained.format, obtained.channels, obtained.freq);

    if (cvt_status < 0) {
        cvt.len_ratio = (double)obtained.freq / (double)pcm_sampr;
    }
}

void pcm_dma_apply_settings(void)
{
    pcm_play_lock();
    pcm_dma_apply_settings_nolock();
    pcm_play_unlock();
}

void pcm_play_dma_start(const void *addr, size_t size)
{

    pcm_data = addr;
    pcm_data_size = size;

    SDL_PauseAudio(0);
}

void pcm_play_dma_stop(void)
{
    SDL_PauseAudio(1);
#ifdef DEBUG
    if (udata.debug != NULL) {
        fclose(udata.debug);
        udata.debug = NULL;
        DEBUGF("Audio debug file closed\n");
    }
#endif
}

static void write_to_soundcard(struct pcm_udata *udata)
{
#ifdef DEBUG
    if (debug_audio && (udata->debug == NULL)) {
        udata->debug = fopen("audiodebug.raw", "abe");
        DEBUGF("Audio debug file open\n");
    }
#endif
    if (cvt.needed) {
        Uint32 rd = udata->num_in;
        Uint32 wr = (double)rd * cvt.len_ratio;

        if (wr > udata->num_out) {
            wr = udata->num_out;
            rd = (double)wr / cvt.len_ratio;

            if (rd > udata->num_in)
            {
                rd = udata->num_in;
                wr = (double)rd * cvt.len_ratio;
            }
        }

        if (wr == 0 || rd == 0)
        {
            udata->num_out = udata->num_in = 0;
            return;
        }

        if (cvt_status > 0) {
            cvt.len = rd * pcm_sample_bytes;
            cvt.buf = (Uint8 *) malloc(cvt.len * cvt.len_mult);

            pcm_copy_buffer(cvt.buf, pcm_data, cvt.len);

            SDL_ConvertAudio(&cvt);
            memcpy(udata->stream, cvt.buf, cvt.len_cvt);

            udata->num_in = cvt.len / pcm_sample_bytes;
            udata->num_out = cvt.len_cvt / pcm_sample_bytes;

#ifdef DEBUG
            if (udata->debug != NULL) {
               fwrite(cvt.buf, sizeof(Uint8), cvt.len_cvt, udata->debug);
            }
#endif
            free(cvt.buf);
        }
        else {
            /* Convert is bad, so do silence */
            Uint32 num = wr*obtained.channels;
            udata->num_in = rd;
            udata->num_out = wr;

            switch (pcm_channel_bytes)
            {
            case 1:
            {
                Uint8 *stream = udata->stream;
                while (num-- > 0)
                    *stream++ = obtained.silence;
                break;
                }
            case 2:
            {
                Uint16 *stream = (Uint16 *)udata->stream;
                while (num-- > 0)
                    *stream++ = obtained.silence;
                break;
                }
            }
#ifdef DEBUG
            if (udata->debug != NULL) {
               fwrite(udata->stream, sizeof(Uint8), wr, udata->debug);
            }
#endif
        }
    } else {
        udata->num_in = udata->num_out = MIN(udata->num_in, udata->num_out);
        pcm_copy_buffer(udata->stream, pcm_data,
                        udata->num_out * pcm_sample_bytes);
#ifdef DEBUG
        if (udata->debug != NULL) {
           fwrite(pcm_data, sizeof(Uint8), udata->num_out * pcm_sample_bytes,
                  udata->debug);
        }
#endif
    }
}

static void sdl_audio_callback(struct pcm_udata *udata, Uint8 *stream, int len)
{
    logf("sdl_audio_callback: len %d, pcm %d\n", len, pcm_data_size);

    bool new_buffer = false;
    udata->stream = stream;

    SDL_LockMutex(audio_lock);

    /* Write what we have in the PCM buffer */
    if (pcm_data_size > 0)
        goto start;

    /* Audio card wants more? Get some more then. */
    while (len > 0) {
        new_buffer = pcm_play_dma_complete_callback(PCM_DMAST_OK, &pcm_data,
                                                    &pcm_data_size);

        if (!new_buffer) {
            DEBUGF("sdl_audio_callback: No Data.\n");
            break;
        }

    start:
        udata->num_in  = pcm_data_size / pcm_sample_bytes;
        udata->num_out = len / pcm_sample_bytes;

        write_to_soundcard(udata);

        udata->num_in  *= pcm_sample_bytes;
        udata->num_out *= pcm_sample_bytes;

        if (new_buffer)
        {
            new_buffer = false;
            pcm_play_dma_status_callback(PCM_DMAST_STARTED);

            if ((size_t)len > udata->num_out)
            {
                int delay = pcm_data_size*250 / pcm_sampr - 1;

                if (delay > 0)
                {
                    SDL_Delay(delay);

                    if (!pcm_is_playing())
                        break;
                }
            }
        }

        pcm_data      += udata->num_in;
        pcm_data_size -= udata->num_in;
        udata->stream += udata->num_out;
        len           -= udata->num_out;
    }

    SDL_UnlockMutex(audio_lock);
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

void pcm_play_dma_init(void)
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO))
    {
        DEBUGF("Could not initialize SDL audio subsystem!\n");
        return;
    }

    audio_lock = SDL_CreateMutex();

    if (!audio_lock)
    {
        panicf("Could not create audio_lock\n");
        return;
    }

#ifdef DEBUG
    udata.debug = NULL;
    if (debug_audio) {
        udata.debug = fopen("audiodebug.raw", "wbe");
        DEBUGF("Audio debug file open\n");
    }
#endif
}

void pcm_play_dma_postinit(void)
{
}
