/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2020 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
#include "../../SDL_internal.h"

#if SDL_AUDIO_DRIVER_ROCKBOX

/* Output raw audio data to a file. */

#if HAVE_STDIO_H
#include <stdio.h>
#endif

#include "SDL_rwops.h"
#include "SDL_timer.h"
#include "SDL_audio.h"
#include "../SDL_audio_c.h"
#include "SDL_rockboxaudio.h"
#include "SDL_log.h"

SDL_AudioDevice *current_dev = NULL;

/* filled in OpenAudio() */
static int16_t silence[1024];

/* debug */
int rbaud_underruns;

/* rockbox callback (in IRQ) */
static void get_more(const void **start, size_t *size)
{
    //LOGF("get_more called");

    SDL_AudioDevice *this = current_dev;

    /* Circularly loop through the buffers starting after the one most
     * recently marked as playing, and find the next one that is
     * marked as filled. */
    for(int i = 1; i < this->hidden->n_buffers; ++i)
    {
        int idx = (this->hidden->current_playing + i) % this->hidden->n_buffers;
        //LOGF("checking buffer %d", idx);
        if(this->hidden->status[idx] == 1)
        {
            /* Play this one. */
            //LOGF("Playing buffer %d (%d[%d])", idx, this->hidden->rb_buf[i], this->spec.size);
            *start = this->hidden->rb_buf[idx];

            /* Historical note: this line below was the source of the
             * last bug preventing SDL2.0 audio playback on
             * 8/8/2020. We were always passing a size of 0 (through
             * the old SDL1.2 `mixlen'). Fixed - FW.
             */
            *size = this->spec.size;

            /* Mark as playing */
            this->hidden->status[idx] = 2;
            this->hidden->current_playing = idx;
            return;
        }
    }

    /* If the loop failed, we're lagging behind, so we play silence. */
    //LOGF("RB AUDIO: no ready buffer; falling back to silence %d (%d %d %d)", rbaud_underruns, this->hidden->status[0], this->hidden->status[1], this->hidden->status[2]);
    *start = silence;
    *size = sizeof(silence);

    ++rbaud_underruns;
}

static void dump_buffers(_THIS, const char *msg) {
    char buf[64];

    char *ptr = buf;
    for(int i = 0; i < this->hidden->n_buffers; i++) {
        ptr += snprintf(ptr, buf + 64 - ptr, "%d ", this->hidden->status[i]);
    }

    //LOGF("%s: buffer state: %s", msg, buf);
}

/* This function waits until it is possible to write a full sound buffer */
static void
ROCKBOXAUDIO_WaitDevice(_THIS)
{
    dump_buffers(this, "WaitDevice");
    //LOGF("RB AUDIO: WaitDevice");
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
                //LOGF("WaitDevice: a buffer is empty");
                return;
            case 2:
                if(++n_playing >= 2)
                {
                    //LOGF("WaitDevice: a buffer is stale");
                    return;
                }
            default:
                break;
            }
        }

        rb->sleep(0);
    }
}

static void
ROCKBOXAUDIO_PlayDevice(_THIS)
{
    dump_buffers(this, "PlayDevice");
    /* There are two cases in which we should be called:
     *  - There is an empty buffer (marked with status = 0)
     *  - There are more than two buffers marked as playing, meaning at least one is stale.
     */

    /* Find the next empty or stale buffer and fill. */
    for(int i = 1; i <= this->hidden->n_buffers; ++i)
    {
        int idx = (this->hidden->current_playing + i) % this->hidden->n_buffers;

        /* Empty or stale. */
        if(this->hidden->status[idx] == 0 ||
           this->hidden->status[idx] == 2) {

            //LOGF("found empty buffer: %d (status: %d)", idx, this->hidden->status[idx]);

            /* probably premature optimization here */
            char *dst = (char*)this->hidden->rb_buf[idx], *src = this->hidden->mixbuf;
            int size = this->spec.size / 2;
            memcpy(dst, src, size);

            this->hidden->status[idx] = 1;
            rb->yield();

            memcpy(dst + size, src + size, this->spec.size - size);

            //LOGF("filled buffer %d (status %d %d %d %d)", idx, this->hidden->status[0], this->hidden->status[1], this->hidden->status[2], this->hidden->status[3]);

            return;
        }
    }

    //LOGF("WARNING: PlayDevice could not find buffer to fill; DROPPING SAMPLES!");
}

static Uint8 *
ROCKBOXAUDIO_GetDeviceBuf(_THIS)
{
    return (this->hidden->mixbuf);
}

static int
ROCKBOXAUDIO_CaptureFromDevice(_THIS, void *buffer, int buflen)
{
    struct SDL_PrivateAudioData *h = this->hidden;
    const int origbuflen = buflen;

    /* TODO */

#if 0
    SDL_Delay(h->io_delay);

    if (h->io) {
        const size_t br = SDL_RWread(h->io, buffer, 1, buflen);
        buflen -= (int) br;
        buffer = ((Uint8 *) buffer) + br;
        if (buflen > 0) {  /* EOF (or error, but whatever). */
            SDL_RWclose(h->io);
            h->io = NULL;
        }
    }
#endif

    /* if we ran out of file, just write silence. */
    SDL_memset(buffer, this->spec.silence, buflen);

    return origbuflen;
}

static void
ROCKBOXAUDIO_FlushCapture(_THIS)
{
    /* no op...we don't advance the file pointer or anything. */
}


static void
ROCKBOXAUDIO_CloseDevice(_THIS)
{
    rb->pcm_play_stop();
    if ( this->hidden->mixbuf != NULL ) {
        SDL_free(this->hidden->mixbuf);
        this->hidden->mixbuf = NULL;
    }

    for(int i = 0; i < this->hidden->n_buffers; ++i)
    {
        if(this->hidden->rb_buf[i])
            SDL_free(this->hidden->rb_buf[i]);
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

static int
ROCKBOXAUDIO_OpenDevice(_THIS, void *handle, const char *devname, int iscapture)
{
    SDL_LogCritical(SDL_LOG_CATEGORY_AUDIO,
                "You are using the SDL rockbox i/o audio driver!\n");

    this->hidden = (struct SDL_PrivateAudioData *) SDL_malloc(sizeof(*this->hidden));
    if (this->hidden == NULL) {
        return SDL_OutOfMemory();
    }
    SDL_zerop(this->hidden);

    if(!freq_ok(this->spec.freq))
    {
        rb->splashf(HZ, "Warning: Unsupported audio rate. Defaulting to %d Hz", RB_SAMPR);

        // switch to default
        this->spec.freq = RB_SAMPR;
    }

    this->spec.format = AUDIO_S16SYS;
    this->spec.channels = 2;

    SDL_CalculateAudioSpec(&this->spec);

    //LOGF("samplerate %d", this->spec.freq);
    rb->pcm_set_frequency(this->spec.freq);

    /* Allocate mixing buffer */
    if (!iscapture) {
        this->hidden->mixbuf = (Uint8 *) SDL_malloc(this->spec.size);
        if (this->hidden->mixbuf == NULL) {
            return SDL_OutOfMemory();
        }
        SDL_memset(this->hidden->mixbuf, this->spec.silence, this->spec.size);


        /* Increase to reduce skipping at the cost of latency. */
        this->hidden->n_buffers = 4;

        /* Buffer 1 will be filled first. */
        this->hidden->current_playing = 0;

        /* allocate buffers */
        for(int i = 0; i < this->hidden->n_buffers; ++i)
        {
            this->hidden->rb_buf[i] = SDL_malloc(this->spec.size);

            if(!this->hidden->rb_buf[i]) {
                SDL_OutOfMemory();
                return -1;
            }

            this->hidden->status[i] = 0; /* empty */
        }

        memset(silence, 0, sizeof(silence));

        current_dev = this;

        rbaud_underruns = 0;

        //LOGF("beginning playback");
        rb->pcm_play_data(get_more, NULL, NULL, 0);

        //LOGF("done");
    }

    /* We're ready to rock and roll. :-) */
    return 0;
}

static void
ROCKBOXAUDIO_DetectDevices(void)
{
    SDL_AddAudioDevice(SDL_FALSE, DEFAULT_OUTPUT_DEVNAME, (void *) 0x1);
    SDL_AddAudioDevice(SDL_TRUE, DEFAULT_INPUT_DEVNAME, (void *) 0x2);
}

static int
ROCKBOXAUDIO_Init(SDL_AudioDriverImpl * impl)
{
    /* Set the function pointers */
    impl->OpenDevice = ROCKBOXAUDIO_OpenDevice;
    impl->WaitDevice = ROCKBOXAUDIO_WaitDevice;
    impl->PlayDevice = ROCKBOXAUDIO_PlayDevice;
    impl->GetDeviceBuf = ROCKBOXAUDIO_GetDeviceBuf;
    impl->CaptureFromDevice = ROCKBOXAUDIO_CaptureFromDevice;
    impl->FlushCapture = ROCKBOXAUDIO_FlushCapture;

    impl->CloseDevice = ROCKBOXAUDIO_CloseDevice;
    impl->DetectDevices = ROCKBOXAUDIO_DetectDevices;

    impl->AllowsArbitraryDeviceNames = 1;
    impl->HasCaptureSupport = SDL_TRUE;

    return 1;   /* this audio target is available. */
}

AudioBootStrap ROCKBOXAUDIO_bootstrap = {
    "rockbox", "rockbox audio driver", ROCKBOXAUDIO_Init, 0
};

#endif /* SDL_AUDIO_DRIVER_ROCKBOX */

/* vi: set ts=4 sw=4 expandtab: */
