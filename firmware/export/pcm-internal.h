/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 * Copyright (C) 2011 by Michael Sevakis
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
#ifndef PCM_INTERNAL_H
#define PCM_INTERNAL_H

struct pcm_peaks
{
    long period;
    long tick;
    uint16_t val[2];
};

void pcm_do_peak_calculation(struct pcm_peaks *peaks, bool active,
                             const void *addr, int count);

/** The following are for internal use between pcm.c and target-
    specific portion **/

/* Called by the bottom layer ISR when more data is needed. Returns non-
 * zero size if more data is to be played. Setting start to NULL
 * forces stop. */
void pcm_play_get_more_callback(void **start, size_t *size);

/* Called by the bottom layer ISR after next transfer has begun in order
   to fill more data for next "get more" callback to implement double-buffered
   callbacks - except for a couple ASM handlers, help drivers to implement
   this functionality with minimal overhead */
static FORCE_INLINE void pcm_play_dma_started_callback(void)
{
    extern void (* pcm_play_dma_started)(void);
    void (* callback)(void) = pcm_play_dma_started;
    if (callback)
        callback();
}

extern unsigned long pcm_curr_sampr;
extern unsigned long pcm_sampr;
extern int pcm_fsel;

#ifdef HAVE_PCM_DMA_ADDRESS
void * pcm_dma_addr(void *addr);
#endif

extern volatile bool pcm_playing;
extern volatile bool pcm_paused;

void pcm_play_dma_lock(void);
void pcm_play_dma_unlock(void);
void pcm_play_dma_init(void) INIT_ATTR;
void pcm_play_dma_start(const void *addr, size_t size);
void pcm_play_dma_stop(void);
void pcm_play_dma_pause(bool pause);
const void * pcm_play_dma_get_peak_buffer(int *count);

void pcm_dma_apply_settings(void);

#ifdef HAVE_RECORDING

/* DMA transfer in is currently active */
extern volatile bool pcm_recording;

/* APIs implemented in the target-specific portion */
void pcm_rec_dma_init(void);
void pcm_rec_dma_close(void);
void pcm_rec_dma_start(void *addr, size_t size);
void pcm_rec_dma_record_more(void *start, size_t size);
void pcm_rec_dma_stop(void);
const void * pcm_rec_dma_get_peak_buffer(void);

#endif /* HAVE_RECORDING */

#endif /* PCM_INTERNAL_H */
