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

/* filled in OpenAudio() */
static int16_t silence[8192];

static void get_more(const void **start, size_t *size)
{
    SDL_AudioDevice *this = current_dev;

    /* see which, if any, are marked as filled */
    if(this && (this->hidden->status[0] == 1 || this->hidden->status[1] == 1))
    {
        int idx = this->hidden->status[0] == 1 ? 0 : 1;

        *start = this->hidden->rb_buf[idx];
        *size = this->hidden->mixlen;

        this->hidden->status[idx] = 2;
        this->hidden->current_playing = idx;
    }
    else
    {
        LOGF("RB AUDIO: falling back to silence");
        *start = silence;
        *size = sizeof(silence);
    }
}

/* This function waits until it is possible to call PlayAudio again */
static void ROCKBOXAUD_WaitAudio(_THIS)
{
    while(1)
    {
        /* we have five possible states
           0 0: both empty: fill
           1 0 or 0 1: one filled, one empty: fill
           2 0 or 0 2: one playing, one empty: fill
           1 2 or 2 1: one playing, one filled: wait
           2 2: one playing, one unused: fill */
        switch(this->hidden->status[0] + this->hidden->status[1])
        {
        case 0: /* 0 0 */
        case 2: /* 0 2 */
        case 4: /* 2 2 */
            //LOGF("requesting fill (%d %d)", this->hidden->status[0], this->hidden->status[1]);
            return; /* fill */
        case 1: /* 0 1 */
        case 3: /* 2 1 */
            rb->yield();
            break;
        }
    }
}

/* when this is called, SDL wants us to play the samples in mixbuf */
static void ROCKBOXAUD_PlayAudio(_THIS)
{
    /* there are three cases in which we should be called:
     *  - neither buffer is filled
     *  - both buffers are marked as playing (but one is stale)
     *  - one is playing, one is empty
     */
    /* copy the audio to a real buffer */
    int idx;

    /* both are marked as playing, so we must examine current_playing
     * to figure out which one to write to */
    if(this->hidden->status[0] == 2 && this->hidden->status[1] == 2)
        idx = this->hidden->current_playing == 0 ? 1 : 0;
    else
    {
        if(this->hidden->status[0] == 0)
            idx = 0;
        else
            idx = 1;
    }

    SDL_memcpy(this->hidden->rb_buf[idx], this->hidden->mixbuf, this->hidden->mixlen);

    this->hidden->status[idx] = 1;
    //LOGF("filled buffer %d (status %d %d)", idx, this->hidden->status[0], this->hidden->status[1]);
}

static SDL_AudioDevice *ROCKBOXAUD_CreateDevice(int devindex)
{
        SDL_AudioDevice *this;

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

static int ROCKBOXAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
    /* change to our format */
    spec->format = AUDIO_S16SYS;
    spec->channels = 2;
    spec->freq = RB_SAMPR;

    /* we've changed it */
    SDL_CalculateAudioSpec(spec);

    LOGF("samplerate %d", spec->freq);
    rb->mixer_set_frequency(spec->freq);

    /* Allocate mixing buffer */
    this->hidden->mixlen = spec->size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if ( this->hidden->mixbuf == NULL ) {
        return(-1);
    }
    SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);

    /* fill our two buffers */
    this->hidden->rb_buf[0] = SDL_AllocAudioMem(this->hidden->mixlen);
    this->hidden->rb_buf[1] = SDL_AllocAudioMem(this->hidden->mixlen);

    memset((void*)this->hidden->status, 0, sizeof(this->hidden->status));
    memset(silence, 0, sizeof(silence));

    current_dev = this;

    rb->pcm_play_data(get_more, NULL, NULL, 0);

    /* We're ready to rock and roll. :-) */
    return(0);
}
