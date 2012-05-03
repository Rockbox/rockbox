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

#include "config.h"

/* Cheapo buffer align macro to align to the 16-16 PCM size */
#define ALIGN_AUDIOBUF(start, size) \
    ({ (start) = (void *)(((uintptr_t)(start) + 3) & ~3); \
       (size) &= ~3; })

void pcm_do_peak_calculation(struct pcm_peaks *peaks, bool active,
                             const void *addr, int count);

/** The following are for internal use between pcm.c and target-
    specific portion **/
static FORCE_INLINE enum pcm_dma_status pcm_call_status_cb(
    pcm_status_callback_type callback, enum pcm_dma_status status)
{
    if (!callback)
        return status;

    return callback(status);
}

static FORCE_INLINE enum pcm_dma_status
pcm_play_dma_status_callback(enum pcm_dma_status status)
{
    extern enum pcm_dma_status
        (* volatile pcm_play_status_callback)(enum pcm_dma_status);
    return pcm_call_status_cb(pcm_play_status_callback, status);
}

/* Called by the bottom layer ISR when more data is needed. Returns true
 * if a new buffer is available, false otherwise. */
bool pcm_play_dma_complete_callback(enum pcm_dma_status status,
                                    const void **addr, size_t *size);

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
void pcm_play_dma_postinit(void);
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
void pcm_rec_dma_stop(void);
const void * pcm_rec_dma_get_peak_buffer(void);

static FORCE_INLINE enum pcm_dma_status
pcm_rec_dma_status_callback(enum pcm_dma_status status)
{
    extern enum pcm_dma_status
        (* volatile pcm_rec_status_callback)(enum pcm_dma_status);
    return pcm_call_status_cb(pcm_rec_status_callback, status);
}


/* Called by the bottom layer ISR when more data is needed. Returns true
 * if a new buffer is available, false otherwise. */
bool pcm_rec_dma_complete_callback(enum pcm_dma_status status,
                                   void **addr, size_t *size);

#ifdef HAVE_PCM_REC_DMA_ADDRESS
#define pcm_rec_dma_addr(addr) pcm_dma_addr(addr)
#else
#define pcm_rec_dma_addr(addr) addr
#endif

#endif /* HAVE_RECORDING */

#endif /* PCM_INTERNAL_H */
