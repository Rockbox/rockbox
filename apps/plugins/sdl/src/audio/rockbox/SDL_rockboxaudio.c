/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org

    This file written by Ryan C. Gordon (icculus@icculus.org)
*/
#include "SDL_config.h"

/* Output raw audio data to a file. */

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#include "SDL_rwops.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audiomem.h"
#include "../SDL_audio_c.h"
#include "../SDL_audiodev_c.h"
#include "SDL_rockboxaudio.h"

/* The tag name used by DISK audio */
#define ROCKBOXAUD_DRIVER_NAME         "rockbox"

/* Audio driver functions */
static int ROCKBOXAUD_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void ROCKBOXAUD_WaitAudio(_THIS);
static void ROCKBOXAUD_PlayAudio(_THIS);
static Uint8 *ROCKBOXAUD_GetAudioBuf(_THIS);
static void ROCKBOXAUD_CloseAudio(_THIS);

/* Audio driver bootstrap functions */
static int ROCKBOXAUD_Available(void)
{
    /* always! */
    return 1;
}

static void ROCKBOXAUD_DeleteDevice(SDL_AudioDevice *device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

SDL_AudioDevice *current_dev = NULL;

static int16_t silence[8192] = { 0 };

static void get_more(const void **start, size_t *size)
{
    SDL_AudioDevice *this = current_dev;
    if(this && this->hidden->bytes_left > 0)
    {
        *start = this->hidden->rb_buf;
        *size = this->hidden->mixlen;
        this->hidden->bytes_left = 0;
    }
    else
    {
        LOGF("RB AUDIO: falling back to silence");
        *start = silence;
        *size = sizeof(silence);
    }
}

static SDL_AudioDevice *ROCKBOXAUD_CreateDevice(int devindex)
{
        SDL_AudioDevice *this;
        const char *envr;

        /* Initialize all variables that we clean on shutdown */
        this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
        if ( this ) {
                SDL_memset(this, 0, (sizeof *this));
                this->hidden = (struct SDL_PrivateAudioData *)
                                SDL_malloc((sizeof *this->hidden));
        }
        if ( (this == NULL) || (this->hidden == NULL) ) {
                SDL_OutOfMemory();
                if ( this ) {
                        SDL_free(this);
                }
                return(0);
        }
        SDL_memset(this->hidden, 0, (sizeof *this->hidden));

        /* Set the function pointers */
        this->OpenAudio = ROCKBOXAUD_OpenAudio;
        this->WaitAudio = ROCKBOXAUD_WaitAudio;
        this->PlayAudio = ROCKBOXAUD_PlayAudio;
        this->GetAudioBuf = ROCKBOXAUD_GetAudioBuf;
        this->CloseAudio = ROCKBOXAUD_CloseAudio;

        this->free = ROCKBOXAUD_DeleteDevice;

        return this;
}

AudioBootStrap ROCKBOXAUD_bootstrap = {
        ROCKBOXAUD_DRIVER_NAME, "rockbox audio",
        ROCKBOXAUD_Available, ROCKBOXAUD_CreateDevice
};

/* This function waits until it is possible to write a full sound buffer */
static void ROCKBOXAUD_WaitAudio(_THIS)
{
    while(this->hidden->bytes_left)
        rb->yield();
}

static void ROCKBOXAUD_PlayAudio(_THIS)
{
    /* copy the audio to a real buffer */
    SDL_memcpy(this->hidden->rb_buf, this->hidden->mixbuf, this->hidden->mixlen);
    this->hidden->bytes_left = this->hidden->mixlen;
}

static Uint8 *ROCKBOXAUD_GetAudioBuf(_THIS)
{
        return(this->hidden->mixbuf);
}

static void ROCKBOXAUD_CloseAudio(_THIS)
{
    if ( this->hidden->mixbuf != NULL ) {
        SDL_FreeAudioMem(this->hidden->mixbuf);
        this->hidden->mixbuf = NULL;
    }
    rb->pcm_play_stop();
}

#define SAMPLE_RATE HW_SAMPR_DEFAULT

static int ROCKBOXAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
    /* change to our format */
    spec->freq = SAMPLE_RATE;
    spec->format = AUDIO_S16SYS;
    spec->channels = 2;

    /* we've changed it */
    SDL_CalculateAudioSpec(spec);

    /* Allocate mixing buffer */
    this->hidden->mixlen = spec->size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if ( this->hidden->mixbuf == NULL ) {
        return(-1);
    }
    SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);

    /* our real buffer */
    this->hidden->rb_buf = SDL_AllocAudioMem(this->hidden->mixlen);
    this->hidden->bytes_left = 0;

    current_dev = this;

    rb->pcm_play_data(get_more, NULL, NULL, 0);

    /* We're ready to rock and roll. :-) */
    return(0);
}
