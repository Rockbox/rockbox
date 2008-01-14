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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
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
    #warning function not implemented
}

const void * pcm_play_dma_get_peak_buffer(int *count)
{
    #warning function not implemented
    (void) count;
    return 0;
}

void pcm_play_dma_init(void)
{
    #warning function not implemented
}

void pcm_apply_settings(void)
{
    #warning function not implemented
}

void pcm_set_frequency(unsigned int frequency)
{
    #warning function not implemented
    (void) frequency;
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    #warning function not implemented
    (void) addr;
    (void) size;
}

void pcm_play_dma_stop(void)
{
    #warning function not implemented
}

void pcm_play_lock(void)
{
    #warning function not implemented
}

void pcm_play_unlock(void)
{
    #warning function not implemented
}

void pcm_play_dma_pause(bool pause)
{
    #warning function not implemented
    (void) pause;
}

size_t pcm_get_bytes_waiting(void)
{
    #warning function not implemented
    return 0;
}
