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

#include "pcm-internal.h"
#include "pcm_sampr.h"

/*#define LOGF_ENABLE*/
#include "logf.h"

#ifdef DEBUG
#include <stdio.h>
extern bool debug_audio;
#endif

#if CONFIG_CODEC == SWCODEC
static int cvt_status = -1;

static const void *pcm_data;
static unsigned long pcm_data_frames;
static size_t in_frame_size;
static size_t out_frame_size;
static size_t out_sample_size;
static unsigned long pcm_sampr;

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
static bool audio_running = false;
static SDL_mutex *audio_lock;

void pcm_play_dma_lock(void)
{
    SDL_LockMutex(audio_lock);
}

void pcm_play_dma_unlock(void)
{
    SDL_UnlockMutex(audio_lock);
}

static void pcm_dma_apply_settings_nolock(void)
{
    cvt_status = SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, 2, pcm_sampr,
                    obtained.format, obtained.channels, obtained.freq);

    if (cvt_status < 0) {
        cvt.len_ratio = (double)obtained.freq / (double)pcm_sampr;
    }
}

void pcm_dma_apply_settings(const struct pcm_hw_settings *settings)
{
    SDL_LockMutex(audio_lock);
    pcm_sampr = settings->samplerate;
    pcm_dma_apply_settings_nolock();
    SDL_UnlockMutex(audio_lock);
}

void pcm_play_dma_prepare(void)
{
    pcm_play_dma_stop();
    pcm_dma_apply_settings_nolock();
}

void pcm_play_dma_stop(void)
{
    SDL_PauseAudio(1);
    audio_running = false;
    pcm_data = NULL;
    pcm_data_frames = 0;
#ifdef DEBUG
    if (udata.debug != NULL) {
        fclose(udata.debug);
        udata.debug = NULL;
        DEBUGF("Audio debug file closed\n");
    }
#endif
}

unsigned long pcm_play_dma_get_frames_waiting(void)
{
    return pcm_data_frames;
}

static void write_to_soundcard(struct pcm_udata *udata)
{
#ifdef DEBUG
    if (debug_audio && (udata->debug == NULL)) {
        udata->debug = fopen("audiodebug.raw", "ab");
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
            size_t size_rd = rd * in_frame_size;
            size_t size_wr = wr * in_frame_size;

            Uint8 buf[MAX(size_rd, size_wr)];
            cvt.buf = buf;
            cvt.len = size_rd;

            memcpy(buf, pcm_data, size_rd);

            SDL_ConvertAudio(&cvt);
            memcpy(udata->stream, cvt.buf, cvt.len_cvt);

            udata->num_in = cvt.len / in_frame_size;
            udata->num_out = cvt.len_cvt / out_frame_size;

#ifdef DEBUG
            if (udata->debug != NULL) {
               fwrite(cvt.buf, sizeof(Uint8), cvt.len_cvt, udata->debug);
            }
#endif
        }
        else {
            /* Convert is bad, so do silence */
            Uint32 num = wr*obtained.channels;
            udata->num_in = rd;
            udata->num_out = wr;

            switch (out_sample_size)
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
        memcpy(udata->stream, pcm_data, udata->num_out * out_frame_size);
#ifdef DEBUG
        if (udata->debug != NULL) {
           fwrite(pcm_data, sizeof(Uint8), udata->num_out * out_frame_size,
                  udata->debug);
        }
#endif
    }
}

void pcm_play_dma_send_frames(const void *addr, unsigned long frames)
{
    pcm_data = addr;
    pcm_data_frames = frames;

    if (audio_running)
        return;

    SDL_PauseAudio(0);
    audio_running = true;
}

static void sdl_audio_callback(struct pcm_udata *udata, Uint8 *stream, int len)
{
    logf("sdl_audio_callback: len %d, pcm %u\n", len, pcm_data_frames);

    bool new_buffer = false;
    udata->stream = stream;

    SDL_LockMutex(audio_lock);

    /* Audio card wants more? Get some more then. */
    while (len > 0) {
        if (!pcm_data_frames)
        {
            pcm_play_dma_complete_callback(0);

            new_buffer = pcm_data && pcm_data_frames;
            if (!new_buffer) {
                DEBUGF("sdl_audio_callback: No Data.\n");
                break;
            }
        }

        udata->num_in  = pcm_data_frames;
        udata->num_out = len / out_frame_size;

        write_to_soundcard(udata);

        udata->num_out *= out_frame_size;

        if (new_buffer)
        {
            new_buffer = false;
            if ((Uint32)len > udata->num_out)
            {
                /* provide stable frame rate */
                int delay = pcm_data_frames*1000 / pcm_sampr - 1;

                if (delay > 0)
                {
                    SDL_UnlockMutex(audio_lock);
                    SDL_Delay(delay);
                    SDL_LockMutex(audio_lock);

                    if (!audio_running)
                        break;
                }
            }
        }

        pcm_data        += udata->num_in*in_frame_size;
        pcm_data_frames -= udata->num_in;
        udata->stream   += udata->num_out;
        len             -= udata->num_out;
    }

    SDL_UnlockMutex(audio_lock);
}

#ifdef HAVE_RECORDING
void pcm_rec_dma_lock(void)
{
}

void pcm_rec_dma_unlock(void)
{
}

void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_capture_frames(void *addr, unsigned long frames)
{
    (void)addr;
    (void)frames;
}

void pcm_rec_dma_prepare(void)
{
}

void pcm_rec_dma_stop(void)
{
}

unsigned long pcm_rec_dma_get_frames_captured(void)
{
    return 0;
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

void pcm_dma_init(const struct pcm_hw_settings *settings)
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

    SDL_AudioSpec wanted_spec;
#ifdef DEBUG
    udata.debug = NULL;
    if (debug_audio) {
        udata.debug = fopen("audiodebug.raw", "wb");
        DEBUGF("Audio debug file open\n");
    }
#endif
    /* Set 16-bit stereo audio at 44Khz */
    wanted_spec.freq = settings->samplerate;
    wanted_spec.format = AUDIO_S16SYS;
    wanted_spec.channels = 2;
    wanted_spec.samples = 2048;
    wanted_spec.callback =
        (void (SDLCALL *)(void *userdata,
            Uint8 *stream, int len))sdl_audio_callback;
    wanted_spec.userdata = &udata;

    /* Open the audio device and start playing sound! */
    if(SDL_OpenAudio(&wanted_spec, &obtained) < 0) {
        DEBUGF("Unable to open audio: %s\n", SDL_GetError());
        return;
    }

    switch (obtained.format)
    {
    case AUDIO_U8:
    case AUDIO_S8:
        out_sample_size = 1;
        break;
    case AUDIO_U16LSB:
    case AUDIO_S16LSB:
    case AUDIO_U16MSB:
    case AUDIO_S16MSB:
        out_sample_size = 2;
        break;
    default:
        DEBUGF("Unknown sample format obtained: %u\n",
                (unsigned)obtained.format);
        return;
    }

    in_frame_size = PCM_DMA_T_FRAME_SIZE;
    out_frame_size = obtained.channels * out_sample_size;

    pcm_sampr = settings->samplerate;
    pcm_dma_apply_settings_nolock();
}

#endif /* CONFIG_CODEC == SWCODEC */
