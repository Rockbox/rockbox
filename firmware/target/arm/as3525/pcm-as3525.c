/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright © 2008 Rafaël Carré
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
#include "audio.h"
#include "string.h"

/* TODO */

void pcm_play_lock(void)
{
}

void pcm_play_unlock(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
}

void pcm_play_dma_stop(void)
{
}

void pcm_play_dma_pause(bool pause)
{
}

unsigned long physical_address(void *p)
{
    return 0;
}

void pcm_play_dma_init(void)
{
}

void pcm_postinit(void)
{
}

void pcm_set_frequency(unsigned int frequency)
{
}

void pcm_apply_settings(void)
{
}

size_t pcm_get_bytes_waiting(void)
{
    return 0;
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    return NULL;
}


/****************************************************************************
 ** Recording DMA transfer
 **/
#ifdef HAVE_RECORDING
void pcm_rec_lock(void)
{
}

void pcm_rec_unlock(void)
{
}

void pcm_record_more(void *start, size_t size)
{
    (void)start;
    (void)size;
}

void pcm_rec_dma_stop(void)
{
}

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void)addr;
    (void)size;
}

void pcm_rec_dma_close(void)
{
}


void pcm_rec_dma_init(void)
{
}


const void * pcm_rec_dma_get_peak_buffer(int *count)
{
    (void)count;
}

#endif /* HAVE_RECORDING */
