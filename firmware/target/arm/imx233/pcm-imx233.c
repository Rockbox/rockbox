/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2011 by Amaury Pouly
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

#include "config.h"
#include "audiohw.h"
#include "pcm.h"
#include "audioin-imx233.h"
#include "audioout-imx233.h"

void pcm_play_lock(void)
{
}

void pcm_play_unlock(void)
{
}

void pcm_play_dma_stop(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    pcm_play_dma_stop();

    (void) addr;
    (void) size;
}

void pcm_play_dma_pause(bool pause)
{
    (void) pause;
}

void pcm_play_dma_init(void)
{
    audiohw_preinit();
}

void pcm_play_dma_postinit(void)
{
    audiohw_postinit();
}

void pcm_dma_apply_settings(void)
{
    return;
}

size_t pcm_get_bytes_waiting(void)
{
    return 0;
}

const void *pcm_play_dma_get_peak_buffer(int *count)
{
    (void) count;
    return NULL;
}

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

void pcm_rec_dma_start(void *addr, size_t size)
{
    (void) addr;
    (void) size;
}

/*
void pcm_rec_dma_record_more(void *start, size_t size)
{
}
*/

void pcm_rec_dma_stop(void)
{
}

const void * pcm_rec_dma_get_peak_buffer(void)
{
    return NULL;
}
