/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 by Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdlib.h>
#include "system.h"
#include "kernel.h"
#include "logf.h"
#include "audio.h"
#include "sound.h"
#include "file.h"
#include "mmu-imx31.h"

#if 0
static int pcm_freq = HW_SAMPR_DEFAULT; /* 44.1 is default */
#endif

void pcm_play_lock(void)
{
}

void pcm_play_unlock(void)
{
}

#if 0
static void _pcm_apply_settings(void)
{
}
#endif

void pcm_apply_settings(void)
{
}

void pcm_play_dma_init(void)
{
}

void pcm_postinit(void)
{
}

#if 0
/* Connect the DMA and start filling the FIFO */
static void play_start_pcm(void)
{
}

/* Disconnect the DMA and wait for the FIFO to clear */
static void play_stop_pcm(void)
{
}
#endif

void pcm_play_dma_start(const void *addr, size_t size)
{
    (void)addr;
    (void)size;
}

void pcm_play_dma_stop(void)
{
}

void pcm_play_dma_pause(bool pause)
{
    (void)pause;
}

/* Set the pcm frequency hardware will use when play is next started or
   when pcm_apply_settings is called. Do not apply the setting to the
   hardware here but simply cache it. */
void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
}

/* Return the number of bytes waiting - full L-R sample pairs only */
size_t pcm_get_bytes_waiting(void)
{
    return 0;
}

/* Return a pointer to the samples and the number of them in *count */
const void * pcm_play_dma_get_peak_buffer(int *count)
{
    (void)count;
    return NULL;
}

/* Any recording functionality should be implemented similarly */
