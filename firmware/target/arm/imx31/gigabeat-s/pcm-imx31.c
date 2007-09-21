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

void fiq_handler(void) __attribute__((naked));

static void _pcm_apply_settings(void)
{
}

void pcm_apply_settings(void)
{
}

void pcm_init(void)
{
}

void pcm_postinit(void)
{
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    (void)addr;
    (void)size;
}

static void pcm_play_dma_stop_fiq(void)
{
}

void fiq_handler(void)
{
}

/* Disconnect the DMA and wait for the FIFO to clear */
void pcm_play_dma_stop(void)
{
}

void pcm_play_pause_pause(void)
{
}

void pcm_play_pause_unpause(void)
{
}

void pcm_set_frequency(unsigned int frequency)
{
    (void)frequency;
}

size_t pcm_get_bytes_waiting(void)
{
}

void pcm_mute(bool mute)
{
}

/**
 * Return playback peaks - Peaks ahead in the DMA buffer based upon the
 *                         calling period to attempt to compensate for
 *                         delay.
 */
void pcm_calculate_peaks(int *left, int *right)
{
}
