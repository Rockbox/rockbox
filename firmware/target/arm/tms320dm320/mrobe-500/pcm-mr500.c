/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Karl Kurbjun
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
#include "file.h"

void pcm_postinit(void)
{

}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    (void) count;
    return 0;
}

void pcm_play_dma_init(void)
{

}

void pcm_apply_settings(void)
{

}

void pcm_set_frequency(unsigned int frequency)
{
    (void) frequency;
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    (void) addr;
    (void) size;
}

void pcm_play_dma_stop(void)
{

}

void pcm_play_lock(void)
{

}

void pcm_play_unlock(void)
{

}

void pcm_play_dma_pause(bool pause)
{
    (void) pause;
}

size_t pcm_get_bytes_waiting(void)
{
    return 0;
}
