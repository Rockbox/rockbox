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
#include <memory.h>
#include "debug.h"
#include "kernel.h"
#include "sound.h"

#include "pcm.h"
#include "pcm_sampr.h"
#include "SDL.h"

static int cvt_status = -1;

static Uint8* pcm_data;
static size_t pcm_data_size;
static size_t pcm_sample_bytes;
static size_t pcm_channel_bytes;

struct pcm_udata
{
    Uint8 *stream;
    Uint32 num_in;
    Uint32 num_out;
    FILE  *debug;
} udata;

static SDL_AudioSpec obtained;
static SDL_AudioCVT cvt;

extern bool debug_audio;

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

void pcm_play_lock(void)
{
    SDL_LockAudio();
}

void pcm_play_unlock(void)
{
    SDL_UnlockAudio();
}

static void pcm_dma_apply_settings_nolock(void)
{
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
    pcm_dma_apply_settings_nolock();

    pcm_data = (Uint8 *) addr;
    pcm_data_size = size;

    SDL_PauseAudio(0);
}

void pcm_play_dma_stop(void)
{
    SDL_PauseAudio(1);
}

void pcm_play_dma_pause(bool pause)
{
    if (pause)
        SDL_PauseAudio(1);
    else
        SDL_PauseAudio(0);
}

size_t pcm_get_bytes_waiting(void)
{
    return pcm_data_size;
}

extern int sim_volume; /* in firmware/sound.c */
void write_to_soundcard(struct pcm_udata *udata) {
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

            memcpy(cvt.buf, pcm_data, cvt.len);

            SDL_ConvertAudio(&cvt);
            SDL_MixAudio(udata->stream, cvt.buf, cvt.len_cvt, sim_volume);

            udata->num_in = cvt.len / pcm_sample_bytes;
            udata->num_out = cvt.len_cvt / pcm_sample_bytes;

            if (udata->debug != NULL) {
               fwrite(cvt.buf, sizeof(Uint8), cvt.len_cvt, udata->debug);
            }

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

            if (udata->debug != NULL) {
               fwrite(udata->stream, sizeof(Uint8), wr, udata->debug);
            }
        }
    } else {
        udata->num_in = udata->num_out = MIN(udata->num_in, udata->num_out);
        SDL_MixAudio(udata->stream, pcm_data, 
                     udata->num_out * pcm_sample_bytes, sim_volume);
        
        if (udata->debug != NULL) {
           fwrite(pcm_data, sizeof(Uint8), udata->num_out * pcm_sample_bytes,
                  udata->debug);
        }
    }
}

void sdl_audio_callback(struct pcm_udata *udata, Uint8 *stream, int len)
{
    udata->stream = stream;

    /* Write what we have in the PCM buffer */
    if (pcm_data_size > 0)
        goto start;

    /* Audio card wants more? Get some more then. */
    while (len > 0) {
        if ((ssize_t)pcm_data_size <= 0) {
            pcm_data_size = 0;

            if (pcm_callback_for_more)
                pcm_callback_for_more(&pcm_data, &pcm_data_size);
        }

        if (pcm_data_size > 0) {
    start:
            udata->num_in  = pcm_data_size / pcm_sample_bytes;
            udata->num_out = len / pcm_sample_bytes;

            write_to_soundcard(udata);

            udata->num_in  *= pcm_sample_bytes;
            udata->num_out *= pcm_sample_bytes;

            pcm_data      += udata->num_in;
            pcm_data_size -= udata->num_in;
            udata->stream += udata->num_out;
            len           -= udata->num_out;
        } else {
            DEBUGF("sdl_audio_callback: No Data.\n");
            pcm_play_dma_stop();
            pcm_play_dma_stopped_callback();
            break;
        }
    }
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    uintptr_t addr = (uintptr_t)pcm_data;
    *count = pcm_data_size / 4;
    return (void *)((addr + 2) & ~3);
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

void pcm_record_more(void *start, size_t size)
{
    (void)start;
    (void)size;
}

unsigned long pcm_rec_status(void)
{
    return 0;
}

const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    *count = 0;
    return NULL;
}

#endif /* HAVE_RECORDING */

void pcm_play_dma_init(void)
{
    SDL_AudioSpec wanted_spec;
    udata.debug = NULL;

    if (debug_audio) {
        udata.debug = fopen("audiodebug.raw", "wb");
    }

    /* Set 16-bit stereo audio at 44Khz */
    wanted_spec.freq = 44100;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 2;
    wanted_spec.samples = 2048;
    wanted_spec.callback =
        (void (SDLCALL *)(void *userdata,
            Uint8 *stream, int len))sdl_audio_callback;
    wanted_spec.userdata = &udata;

    /* Open the audio device and start playing sound! */
    if(SDL_OpenAudio(&wanted_spec, &obtained) < 0) {
        fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
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
        fprintf(stderr, "Unknown sample format obtained: %u\n",
                (unsigned)obtained.format);
        return;
    }

    pcm_sample_bytes = obtained.channels * pcm_channel_bytes;

    pcm_dma_apply_settings_nolock();
}

void pcm_postinit(void)
{
}
