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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "autoconf.h"

#ifdef ROCKBOX_HAS_SIMSOUND /* play sound in sim enabled */

#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include "debug.h"
#include "kernel.h"
#include "sound.h"
#include "SDL.h"

static bool pcm_playing;
static bool pcm_paused;

static Uint8* pcm_data;
static Uint32 pcm_data_size;

extern bool debug_audio;

static void sdl_dma_start(const void *addr, size_t size)
{
    pcm_playing = true;

    SDL_LockAudio();
    
    pcm_data = (Uint8 *) addr;
    pcm_data_size = size;

    SDL_UnlockAudio();
    
    SDL_PauseAudio(0);
}

static void sdl_dma_stop() 
{
    pcm_playing = false;

    SDL_PauseAudio(1);

    pcm_paused = false;
}

static void (*callback_for_more)(unsigned char**, size_t*) = NULL;
void pcm_play_data(void (*get_more)(unsigned char** start, size_t* size),
        unsigned char* start, size_t size)
{
    callback_for_more = get_more;

    if (!(start && size)) {
        if (get_more)
            get_more(&start, &size);
        else
            return;
    }

    if (start && size) {
        sdl_dma_start(start, size);
    }
}

size_t pcm_get_bytes_waiting(void)
{
    return pcm_data_size;
}

void pcm_mute(bool mute)
{
    (void) mute;
}

void pcm_play_stop(void)
{
    if (pcm_playing) {
        sdl_dma_stop();
    }
}

void pcm_play_pause(bool play)
{
    Uint32 next_size;
    Uint8 *next_start;
    
    if (!pcm_playing) {
        return;
    }

    if(pcm_paused && play) {
        if (pcm_get_bytes_waiting()) {
            printf("unpause\n");

            SDL_PauseAudio(0);
        } else {
            printf("unpause, no data waiting\n");

            void (*get_more)(unsigned char**, size_t*) = callback_for_more;

            if (get_more) {
                get_more(&next_start, &next_size);
            }

            if (next_start && next_size) {
                sdl_dma_start(next_start, next_size);
            } else {
                sdl_dma_stop();
                printf("unpause attempted, no data\n");
            }
        }
    } else if(!pcm_paused && !play) {
        printf("pause\n");

        SDL_PauseAudio(1);
    }

    pcm_paused = !play;
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

/*
 * This function goes directly into the DMA buffer to calculate the left and
 * right peak values. To avoid missing peaks it tries to look forward two full
 * peek periods (2/HZ sec, 100% overlap), although it's always possible that
 * the entire period will not be visible. To reduce CPU load it only looks at
 * every third sample, and this can be reduced even further if needed (even
 * every tenth sample would still be pretty accurate).
 */

#define PEAK_SAMPLES  (44100*2/HZ)  /* 44100 samples * 2 / 100 Hz tick */
#define PEAK_STRIDE   3       /* every 3rd sample is plenty... */

void pcm_calculate_peaks(int *left, int *right)
{
    long samples = (long) pcm_data_size / 4;
    short *addr = (short *) pcm_data;

    if (samples > PEAK_SAMPLES)
        samples = PEAK_SAMPLES;

    samples /= PEAK_STRIDE;

    if (left && right) {
        int left_peak = 0, right_peak = 0, value;

        while (samples--) {
            if ((value = addr [0]) > left_peak)
                left_peak = value;
            else if (-value > left_peak)
                left_peak = -value;

            if ((value = addr [PEAK_STRIDE | 1]) > right_peak)
                right_peak = value;
            else if (-value > right_peak)
                right_peak = -value;

            addr += PEAK_STRIDE * 2;
        }

        *left = left_peak;
        *right = right_peak;
    }
    else if (left || right) {
        int peak_value = 0, value;

        if (right)
            addr += (PEAK_STRIDE | 1);

        while (samples--) {
            if ((value = addr [0]) > peak_value)
                peak_value = value;
            else if (-value > peak_value)
                peak_value = -value;

            addr += PEAK_STRIDE * 2;
        }

        if (left)
            *left = peak_value;
        else
            *right = peak_value;
    }
}

void sdl_audio_callback(void *udata, Uint8 *stream, int len)
{
    Uint32 have_now;
    FILE *debug = (FILE *)udata;

    /* At all times we need to write a full 'len' bytes to stream. */

    if (pcm_data_size > 0) {
        have_now = (((Uint32)len) > pcm_data_size) ? pcm_data_size : (Uint32)len;

        memcpy(stream, pcm_data, have_now);

        if (debug != NULL) {
            fwrite(pcm_data, sizeof(Uint8), have_now, debug);
        }
        stream += have_now;
        len -= have_now;
        pcm_data += have_now;
        pcm_data_size -= have_now;
    }

    while (len > 0)
    {
        if (callback_for_more) {
            callback_for_more(&pcm_data, &pcm_data_size);
        } else {
            pcm_data = NULL;
            pcm_data_size = 0;
        }
        if (pcm_data_size > 0) {
            have_now = (((Uint32)len) > pcm_data_size) ? pcm_data_size : (Uint32)len;

            memcpy(stream, pcm_data, have_now);

            if (debug != NULL) {
                fwrite(pcm_data, sizeof(Uint8), have_now, debug);
            }
            stream += have_now;
            len -= have_now;
            pcm_data += have_now;
            pcm_data_size -= have_now;
        } else {
            DEBUGF("sdl_audio_callback: No Data.\n");
            sdl_dma_stop();
            break;
        }
    }
}

int pcm_init(void)
{
    SDL_AudioSpec fmt;
    FILE *debug = NULL;

    if (debug_audio) {
        debug = fopen("audiodebug.raw", "wb");
    }

    /* Set 16-bit stereo audio at 44Khz */
    fmt.freq = 44100;
    fmt.format = AUDIO_S16SYS;
    fmt.channels = 2;
    fmt.samples = 2048;
    fmt.callback = sdl_audio_callback;
    fmt.userdata = debug;

    /* Open the audio device and start playing sound! */
    if(SDL_OpenAudio(&fmt, NULL) < 0) {
        fprintf(stderr, "Unable to open audio: %s\n", SDL_GetError());
        return -1;
    }

    sdl_dma_stop();

    return 0;
}

#endif /* ROCKBOX_HAS_SIMSOUND */

