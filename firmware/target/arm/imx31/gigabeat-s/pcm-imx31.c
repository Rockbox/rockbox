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

static int pcm_freq = HW_SAMPR_DEFAULT; /* 44.1 is default */

void fiq_handler(void) __attribute__((naked));

/* Implement separately on recording and playback - simply disable the
   specific DMA interrupt. Disable the FIQ itself only temporarily to sync
   with the DMA interrupt and restore its previous state. The pcm routines
   will call the lockout first then call into these low-level routines. */
struct dma_lock
{
    int locked;
    unsigned long state;
};

static struct dma_play_lock =
{
    .locked = 0,
    .state = 0, /* Initialize this as disabled */
};

void pcm_play_lock(void)
{
    int status = set_fiq_status(FIQ_DISABLED);
    if (++dma_play_lock.locked == 1)
        ; /* Mask the DMA interrupt */
    set_fiq_status(status);
}

void pcm_play_unlock(void)
{
    int status = set_fiq_status(FIQ_DISABLED);
    if (--dma_play_lock.locked == 0)
        ; /* Unmask the DMA interrupt if enabled */
    set_fiq_status(status);
}

static void _pcm_apply_settings(void)
{
    if (pcm_freq != pcm_curr_sampr)
    {
        pcm_curr_sampr = pcm_freq;
        /* Change hardware sample rate */
        /* */
    }
}

void pcm_apply_settings(void)
{
    /* Lockout FIQ and sync the hardware to the settings */
    int oldstatus = set_fiq_status(FIQ_DISABLED);
    _pcm_apply_settings();
    set_fiq_status(oldstatus);
}

void pcm_play_dma_init(void)
{
    pcm_set_frequency(SAMPR_44);

#if 0
    /* Do basic init enable output in pcm_postinit if popping is a
       problem at boot to enable a lenghy delay and let the boot
       process continue */
    audiohw_init();
#endif
}

void pcm_postinit(void)
{
}

/* Connect the DMA and start filling the FIFO */
static void play_start_pcm(void)
{
#if 0
    /* unmask DMA interrupt when unlocking */
    dma_play_lock.state = 0; /* Set to allow pcm_play_unlock to unmask interrupt */
#endif
}

/* Disconnect the DMA and wait for the FIFO to clear */
static void play_stop_pcm(void)
{
#if 0
    /* Keep interrupt masked when unlocking */
    dma_play_lock.state = 0; /* Set to keep pcm_play_unlock from unmasking interrupt */
#endif
}

void pcm_play_dma_start(const void *addr, size_t size)
{
    (void)addr;
    (void)size;
}

void pcm_play_dma_stop(void)
{
    play_stop_pcm();
}

void pcm_play_dma_pause(bool pause)
{
    if (pause)
    {
        play_stop_pcm();
    }
    else
    {
        play_start_pcm();
    }
}

/* Get more samples to play out - call pcm_play_dma_stop and
   pcm_play_dma_stopped_callback if the data runs out */
void fiq_handler(void)
{
#if 0
    /* Callback missing or no more DMA to do */
    pcm_play_dma_stop();
    pcm_play_dma_stopped_callback();
#endif
}

/* Set the pcm frequency hardware will use when play is next started or
   when pcm_apply_settings is called. Do not apply the setting to the
   hardware here but simply cache it. */
void pcm_set_frequency(unsigned int frequency)
{
     pcm_freq = frequency;
}

/* Return the number of bytes waiting - full L-R sample pairs only */
size_t pcm_get_bytes_waiting(void)
{
}

/* Return a pointer to the samples and the number of them in *count */
const void * pcm_play_dma_get_peak_buffer(int *count)
{
    (void)count;
}

/* Any recording functionality should be implemented similarly */
