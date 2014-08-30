/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Thomas Martitz
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


/*
 * Based, but heavily modified, on the example given at
 * http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_8c-example.html
 *
 * This driver uses the so-called unsafe async callback method and hardcoded device
 * names. It fails when the audio device is busy by other apps.
 *
 * To make the async callback safer, an alternative stack is installed, since
 * it's run from a signal hanlder (which otherwise uses the user stack). If
 * tick tasks are run from a signal handler too, please install
 * an alternative stack for it too.
 *
 * TODO: Rewrite this to do it properly with multithreading
 *
 * Alternatively, a version using polling in a tick task is provided. While
 * supposedly safer, it appears to use more CPU (however I didn't measure it
 * accurately, only looked at htop). At least, in this mode the "default"
 * device works which doesnt break with other apps running.
 */


#include "autoconf.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include "tinyalsa/asoundlib.h"
#include "tinyalsa/asound.h"
#include "system.h"
#include "debug.h"
#include "kernel.h"

#include "pcm.h"
#include "pcm-internal.h"
#include "pcm_mixer.h"
#include "pcm_sampr.h"
#include "audiohw.h"

#include <pthread.h>
#include <signal.h>

static const snd_pcm_format_t format = PCM_FORMAT_S16_LE;    /* sample format */
static const int channels = 2;                                /* count of channels */
static unsigned int rate = 44100;                       /* stream rate */

typedef struct pcm snd_pcm_t;
static snd_pcm_t *handle;
struct pcm_config config;

static snd_pcm_sframes_t period_size = 512;  /* if set to >= 1024, all timers become even slower */
static char *frames;

static const void  *pcm_data = 0;
static size_t       pcm_size = 0;

static int recursion;

static int set_hwparams(snd_pcm_t *handle)
{
    int err;
    if (!frames)
        frames = malloc(pcm_frames_to_bytes(handle, pcm_get_buffer_size(handle)));
    err = 0; /* success */
    return err;
}


/* copy pcm samples to a spare buffer, suitable for snd_pcm_writei() */
static bool fill_frames(void)
{
   size_t copy_n, frames_left = period_size;
   bool new_buffer = false;

   while (frames_left > 0)
   {
       if (!pcm_size)
       {
           new_buffer = true;
           if (!pcm_play_dma_complete_callback(PCM_DMAST_OK, &pcm_data,
                                               &pcm_size))
           {
               return false;
           }
       }
       copy_n = MIN((size_t)pcm_size, pcm_frames_to_bytes(handle, frames_left));
       memcpy(&frames[pcm_frames_to_bytes(handle, period_size-frames_left)], pcm_data, copy_n);

       pcm_data += copy_n;
       pcm_size -= copy_n;
       frames_left -= pcm_bytes_to_frames(handle, copy_n);

       if (new_buffer)
       {
           new_buffer = false;
           pcm_play_dma_status_callback(PCM_DMAST_STARTED);
       }
   }
    return true;
}


static void pcm_tick(void)
{
   if (fill_frames())
   {
       if (pcm_write(handle, frames, pcm_frames_to_bytes(handle, period_size))) {
            printf("Error playing sample\n");
            return;//break;
       }

   }
   else
   {
       DEBUGF("%s: No Data.\n", __func__);
       return;//break;
   }
}

static int async_rw(snd_pcm_t *handle)
{
   int err;
   snd_pcm_sframes_t sample_size;
   char *samples;

   /* fill buffer with silence to initiate playback without noisy click */
   sample_size = pcm_frames_to_bytes(handle, pcm_get_buffer_size(handle)); 
   samples = malloc(sample_size);

   memset(samples, 0, sample_size);

   err = pcm_write(handle, samples, sample_size);
   free(samples);

   if (err != 0)
   {
           DEBUGF("Initial write error: %d\n", err);
           return err;
   }
   if (pcm_state(handle) == PCM_STATE_PREPARED)
   {
           err = pcm_start(handle);
           if (err < 0)
           {
               DEBUGF("Start error: %d\n", err);
               return err;
           }
   }
    return 0;
}

void cleanup(void)
{
    free(frames);
    frames = NULL;
    pcm_close(handle);
}

void pcm_play_dma_init(void)
{
    config.channels = channels;
    config.rate = rate;
    config.period_size = period_size;
    config.period_count = 4;
    config.format = format;
    config.start_threshold = 0;
    config.stop_threshold = 0;
    config.silence_threshold = 0;


   handle = pcm_open(0, 0, PCM_OUT, &config);
   if (!handle || !pcm_is_ready(handle)) {
       printf("Unable to open PCM device: %s\n", pcm_get_error(handle));
       return;
   }

    pcm_dma_apply_settings();

    tick_add_task(pcm_tick);

    atexit(cleanup);
    return;
}


void pcm_play_lock(void)
{
    if (recursion++ == 0)
        tick_remove_task(pcm_tick);
}

void pcm_play_unlock(void)
{
    if (--recursion == 0)
        tick_add_task(pcm_tick);
}

static void pcm_dma_apply_settings_nolock(void)
{
    set_hwparams(handle);
}

void pcm_dma_apply_settings(void)
{
    pcm_play_lock();
    pcm_dma_apply_settings_nolock();
    pcm_play_unlock();
}


void pcm_play_dma_pause(bool pause)
{
    (void)pause;
}


void pcm_play_dma_stop(void)
{
    pcm_stop(handle);
}

void pcm_play_dma_start(const void *addr, size_t size)
{
#if defined(DX50) || defined(DX90)
   /* headphone output relay: if this is done at startup already, a loud click is audible on headphones. Here, some time later,
      the output caps are charged a bit and the click is much softer  */
   system("/system/bin/muteopen");
#endif
    pcm_dma_apply_settings_nolock();

    pcm_data = addr;
    pcm_size = size;

   while (1)
   {
       snd_pcm_state_t state = pcm_state(handle);
       switch (state)
       {
           case PCM_STATE_RUNNING:
               return;
           case PCM_STATE_XRUN:
           {
               printf("No handler for STATE_XRUN!\n");
               continue;
           }
           case PCM_STATE_PREPARED:
           {   /* prepared state, we need to fill the buffer with silence before
                * starting */
               int err = async_rw(handle);
               if (err < 0)
                   printf("Start error: %d\n", err);
               return;
           }
           case PCM_STATE_PAUSED:
           {   /* paused, simply resume */
               pcm_play_dma_pause(0);
               return;
           }
           case PCM_STATE_DRAINING:
               /* run until drained */
               continue;
           default:
               DEBUGF("Unhandled state: %d\n", state);
               return;
       }
   }
}

size_t pcm_get_bytes_waiting(void)
{
    return pcm_size;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    uintptr_t addr = (uintptr_t)pcm_data;
    *count = pcm_size / 4;
    return (void *)((addr + 3) & ~3);
}

void pcm_play_dma_postinit(void)
{
   return;
}

void pcm_set_mixer_volume(int volume)
{
#if defined(DX50) || defined(DX90)
    /* -990 to 0 -> 0 to 255 */
    int val = (volume+990)*255/990;
#if defined(DX50)
    FILE *f = fopen("/dev/codec_volume", "w");
#else /* DX90 */
    FILE *f = fopen("/sys/class/codec/es9018_volume", "w");
#endif  /* DX50 */
    fprintf(f, "%d", val);
    fclose(f);
#else
    (void)volume;
#endif /* DX50 || DX90 */
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

#endif /* HAVE_RECORDING */
