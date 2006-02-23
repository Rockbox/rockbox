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

Uint8 overflow[8192];
Uint32 overflow_amount = 0;

void sdl_audio_callback(void *udata, Uint8 *stream, int len)
{
    Uint32 pcm_data_played, need_to_play;
    FILE *debug = (FILE *)udata;

    /* At all times we need to write a full 'len' bytes to stream. */

    if (pcm_data_size > 0) {
        /* We have some PCM data to play. Play as much as we can. */

        pcm_data_played = (((Uint32)len) > pcm_data_size) ? pcm_data_size : (Uint32)len;

        memcpy(stream, pcm_data, pcm_data_played);

        if (debug != NULL) {
            fwrite(pcm_data, sizeof(Uint8), pcm_data_played, debug);
        }
        
        stream += pcm_data_played;
        need_to_play = len - pcm_data_played;
        pcm_data += pcm_data_played;
        pcm_data_size -= pcm_data_played;

        while(need_to_play > 0) {
            /* Loop until we have written enough */

            callback_for_more(&pcm_data, &pcm_data_size);

            if (pcm_data_size > 0) {
                /* We got more data */
                pcm_data_played = (need_to_play > pcm_data_size) ? pcm_data_size : need_to_play;

                memcpy(stream, pcm_data, pcm_data_played);

                if (debug != NULL) {
                    fwrite(pcm_data, sizeof(Uint8), pcm_data_played, debug);
                }

                stream += pcm_data_played;
                need_to_play -= pcm_data_played;
                pcm_data += pcm_data_played;
                pcm_data_size -= pcm_data_played;
            }
        }
    } else {
        pcm_data_size = 0;
        pcm_data = NULL;

        /* No data, try and get some */
        callback_for_more(&pcm_data, &pcm_data_size);
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

