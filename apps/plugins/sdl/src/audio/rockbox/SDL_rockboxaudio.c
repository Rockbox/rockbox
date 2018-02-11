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

  This file written by Franklin Wei (franklin@rockbox.org).
*/
#include "SDL_config.h"

/* SDL Rockbox audio driver */

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

/* debug */
int rbaud_underruns;

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
static int16_t silence[1024];

/* rockbox callback (in IRQ) */
static void get_more(const void **start, size_t *size)
{
    SDL_AudioDevice *this = current_dev;

    /* Circularly loop through the buffers starting after the one most
     * recently marked as playing, and find the next one that is
     * marked as filled. */
    for(int i = 1; i < this->hidden->n_buffers; ++i)
    {
        int idx = (this->hidden->current_playing + i) % this->hidden->n_buffers;
        if(this->hidden->status[idx] == 1)
        {
            /* Play this one. */
            //LOGF("Playing buffer %d", idx);
            *start = this->hidden->rb_buf[idx];
            *size = this->hidden->mixlen;

            /* Mark as playing */
            this->hidden->status[idx] = 2;
            this->hidden->current_playing = idx;
            return;
        }
    }

    /* If the loop failed, we're lagging behind, so we play silence. */
    LOGF("RB AUDIO: falling back to silence %d (%d %d %d)", rbaud_underruns, this->hidden->status[0], this->hidden->status[1], this->hidden->status[2]);
    *start = silence;
    *size = sizeof(silence);

    ++rbaud_underruns;
}

/* This function waits until it is possible to call PlayAudio
 * again. This is called from the SDL_audio thread. */
static void ROCKBOXAUD_WaitAudio(_THIS)
{
    while(1)
    {
        /* We fill in the following cases:
         *  There's an empty buffer (status=0).
         *  There's more than buffers two marked as playing; this
         *  means at least one is stale and needs filling (status=2).
         */
        int n_playing = 0;
        for(int i = 0; i < this->hidden->n_buffers; ++i)
        {
            switch(this->hidden->status[i])
            {
            case 0:
                return;
            case 2:
                if(++n_playing >= 2)
                    return;
            default:
                break;
            }
        }

        rb->sleep(0);
    }
}

/* when this is called, SDL wants us to play the samples in mixbuf */
static void ROCKBOXAUD_PlayAudio(_THIS)
{
    /* There are two cases in which we should be called:
     *  - There is an empty buffer (marked with status = 0)
     *  - There are more than two buffers marked as playing, meaning at least one is stale.
     */
    int idx = -1;

    /* Find the next empty or stale buffer and fill. */
    for(int i = 1; i < this->hidden->n_buffers; ++i)
    {
        idx = (this->hidden->current_playing + i) % this->hidden->n_buffers;

        /* Empty or stale. */
        if(this->hidden->status[idx] == 0 ||
           this->hidden->status[idx] == 2)
            break;
    }
    if(idx < 0)
        return;

    /* probably premature optimization here */
    char *dst = (char*)this->hidden->rb_buf[idx], *src = this->hidden->mixbuf;
    int size = this->hidden->mixlen / 2;
    memcpy(dst, src, size);

    this->hidden->status[idx] = 1;
    rb->yield();

    memcpy(dst + size, src + size, this->hidden->mixlen - size);

    //LOGF("filled buffer %d (status %d %d %d)", idx, this->hidden->status[0], this->hidden->status[1], this->hidden->status[2]);
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
    /* Tell SDL to put samples in mixbuf */
    return(this->hidden->mixbuf);
}

static void ROCKBOXAUD_CloseAudio(_THIS)
{
    rb->pcm_play_stop();
    if ( this->hidden->mixbuf != NULL ) {
        SDL_FreeAudioMem(this->hidden->mixbuf);
        this->hidden->mixbuf = NULL;
    }

    for(int i = 0; i < this->hidden->n_buffers; ++i)
    {
        if(this->hidden->rb_buf[i])
            SDL_FreeAudioMem(this->hidden->rb_buf[i]);
    }
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
}

static bool freq_ok(unsigned int freq)
{
    for(int i = 0; i < SAMPR_NUM_FREQ; i++)
    {
        if(rb->hw_freq_sampr[i] == freq)
            return true;
    }
    return false;
}

static int ROCKBOXAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
    /* change to our format */
    spec->format = AUDIO_S16SYS;
    spec->channels = 2;

    if(!freq_ok(spec->freq))
    {
        rb->splashf(HZ, "Warning: Unsupported audio rate. Defaulting to %d Hz", RB_SAMPR);

        // switch to default
        spec->freq = RB_SAMPR;
    }

    /* we've changed it */
    SDL_CalculateAudioSpec(spec);

    LOGF("samplerate %d", spec->freq);
    rb->pcm_set_frequency(spec->freq);

    /* Allocate mixing buffer */
    this->hidden->mixlen = spec->size;
    this->hidden->mixbuf = (Uint8 *) SDL_AllocAudioMem(this->hidden->mixlen);
    if ( this->hidden->mixbuf == NULL ) {
        return(-1);
    }
    SDL_memset(this->hidden->mixbuf, spec->silence, spec->size);

    /* Increase to reduce skipping at the cost of latency. */
    this->hidden->n_buffers = 4;

    /* Buffer 1 will be filled first. */
    this->hidden->current_playing = 0;

    /* allocate buffers */
    for(int i = 0; i < this->hidden->n_buffers; ++i)
    {
        this->hidden->rb_buf[i] = SDL_AllocAudioMem(this->hidden->mixlen);
        this->hidden->status[i] = 0; /* empty */
    }

    memset(silence, 0, sizeof(silence));

    current_dev = this;

    rbaud_underruns = 0;

    rb->pcm_play_data(get_more, NULL, NULL, 0);

    /* We're ready to rock and roll. :-) */
    return(0);
}
