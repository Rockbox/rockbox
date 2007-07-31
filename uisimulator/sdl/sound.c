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

#include <stdlib.h>
#include <stdbool.h>
#include <memory.h>
#include "debug.h"
#include "kernel.h"
#include "sound.h"

#ifdef HAVE_RECORDING
#ifndef REC_SAMPR_CAPS
#define REC_SAMPR_CAPS  SAMPR_CAP_44
#endif
#endif

#include "pcm_sampr.h"
#include "SDL.h"

static bool pcm_playing;
static bool pcm_paused;
static int cvt_status = -1;
static unsigned long pcm_frequency = SAMPR_44;
static unsigned long pcm_curr_frequency = SAMPR_44;

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

static void pcm_apply_settings_nolock(void)
{
    cvt_status = SDL_BuildAudioCVT(&cvt, AUDIO_S16SYS, 2, pcm_frequency,
                    obtained.format, obtained.channels, obtained.freq);

    pcm_curr_frequency = pcm_frequency;

    if (cvt_status < 0) {
        cvt.len_ratio = (double)obtained.freq / (double)pcm_curr_frequency;
    }
}

void pcm_apply_settings(void)
{
    SDL_LockAudio();
    pcm_apply_settings_nolock();
    SDL_UnlockAudio();
}

static void sdl_dma_start_nolock(const void *addr, size_t size)
{
    pcm_playing = false;

    pcm_apply_settings_nolock();

    pcm_data = (Uint8 *) addr;
    pcm_data_size = size;

    pcm_playing = true;

    SDL_PauseAudio(0);
}

static void sdl_dma_stop_nolock(void) 
{
    pcm_playing = false;

    SDL_PauseAudio(1);

    pcm_paused = false;
}

static void (*callback_for_more)(unsigned char**, size_t*) = NULL;
void pcm_play_data(void (*get_more)(unsigned char** start, size_t* size),
        unsigned char* start, size_t size)
{
    SDL_LockAudio();

    callback_for_more = get_more;

    if (!(start && size)) {
        if (get_more)
            get_more(&start, &size);
    }

    if (start && size) {
        sdl_dma_start_nolock(start, size);
    }

    SDL_UnlockAudio();
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
    SDL_LockAudio();
    if (pcm_playing) {
        sdl_dma_stop_nolock();
    }
    SDL_UnlockAudio();
}

void pcm_play_pause(bool play)
{
    size_t next_size;
    Uint8 *next_start;

    SDL_LockAudio();
    
    if (!pcm_playing) {
        SDL_UnlockAudio();
        return;
    }

    if(pcm_paused && play) {
        if (pcm_get_bytes_waiting()) {
            printf("unpause\n");
            pcm_apply_settings_nolock();
            SDL_PauseAudio(0);
        } else {
            printf("unpause, no data waiting\n");

            void (*get_more)(unsigned char**, size_t*) = callback_for_more;

            if (get_more) {
                get_more(&next_start, &next_size);
            }

            if (next_start && next_size) {
                sdl_dma_start_nolock(next_start, next_size);
            } else {
                sdl_dma_stop_nolock();
                printf("unpause attempted, no data\n");
            }
        }
    } else if(!pcm_paused && !play) {
        printf("pause\n");

        SDL_PauseAudio(1);
    }

    pcm_paused = !play;

    SDL_UnlockAudio();
}

bool pcm_is_paused(void)
{
    return pcm_paused;
}

bool pcm_is_playing(void)
{
    return pcm_playing;
}

void pcm_set_frequency(unsigned int frequency)
{
    switch (frequency)
    {
    HW_HAVE_8_( case SAMPR_8:)
    HW_HAVE_11_(case SAMPR_11:)
    HW_HAVE_12_(case SAMPR_12:)
    HW_HAVE_16_(case SAMPR_16:)
    HW_HAVE_22_(case SAMPR_22:)
    HW_HAVE_24_(case SAMPR_24:)
    HW_HAVE_32_(case SAMPR_32:)
    HW_HAVE_44_(case SAMPR_44:)
    HW_HAVE_48_(case SAMPR_48:)
    HW_HAVE_64_(case SAMPR_64:)
    HW_HAVE_88_(case SAMPR_88:)
    HW_HAVE_96_(case SAMPR_96:)
        break;
    default:
        frequency = SAMPR_44;
    }

    pcm_frequency = frequency;
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

            if (callback_for_more)
                callback_for_more(&pcm_data, &pcm_data_size);
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
            sdl_dma_stop_nolock();
            break;
        }
    }
}

#ifdef HAVE_RECORDING
void pcm_init_recording(void)
{
}

void pcm_close_recording(void)
{
}

void pcm_record_data(void (*more_ready)(void* start, size_t size),
                     void *start, size_t size)
{
    (void)more_ready;
    (void)start;
    (void)size;
}

void pcm_stop_recording(void)
{
}

void pcm_record_more(void *start, size_t size)
{
    (void)start;
    (void)size;
}

void pcm_calculate_rec_peaks(int *left, int *right)
{
    if (left)
        *left = 0;
    if (right)
        *right = 0;
}

unsigned long pcm_rec_status(void)
{
    return 0;
}

#endif /* HAVE_RECORDING */

int pcm_init(void)
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
        return -1;
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
        return -1;
    }

    pcm_sample_bytes = obtained.channels * pcm_channel_bytes;

    pcm_apply_settings_nolock();

    sdl_dma_stop_nolock();

    return 0;
}

void pcm_postinit(void)
{
}
