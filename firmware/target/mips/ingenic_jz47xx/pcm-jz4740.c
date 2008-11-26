/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Maurus Cuelenaere
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

#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "pcm.h"
#include "jz4740.h"

/****************************************************************************
 ** Playback DMA transfer
 **/

void pcm_postinit(void)
{
    audiohw_postinit(); /* implemented not for all codecs */
    pcm_apply_settings();
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    /* TODO */
    *count = 0;
    return NULL;
}

void pcm_play_dma_init(void)
{
    /* TODO */

    /* Initialize default register values. */
    audiohw_init();
}

void pcm_postinit(void)
{
    audiohw_postinit();
}

void pcm_apply_settings(void)
{
    /* TODO */
}

void pcm_set_frequency(unsigned int frequency)
{
    (void) frequency;
    /* TODO */
}

static void play_start_pcm(void)
{
    pcm_apply_settings();

    /* TODO */
}

static void play_stop_pcm(void)
{
    /* TODO */
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    (void)addr;
    (void)size;
    /* TODO */

    play_start_pcm();
}

void pcm_play_dma_stop(void)
{
    play_stop_pcm();
    
    /* TODO */
}

void pcm_play_lock(void)
{
    /* TODO */
}

void pcm_play_unlock(void)
{
   /* TODO */
}

void pcm_play_dma_pause(bool pause)
{
    if(pause)
        play_stop_pcm();
    else
        play_start_pcm();
    
}

size_t pcm_get_bytes_waiting(void)
{
    /* TODO */
    return 0;
}

#ifdef HAVE_RECORDING
/* TODO */
void pcm_rec_dma_init(void)
{
}

void pcm_rec_dma_close(void)
{
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void) addr;
    (void) size;
}

void pcm_rec_dma_stop(void)
{
}

void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    *count = 0;
    return NULL;
}

void pcm_record_more(void *start, size_t size)
{
    (void) start;
    (void) size;
}
#endif
