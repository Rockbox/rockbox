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
#ifndef PCM_PLAYBACK_H
#define PCM_PLAYBACK_H

#include <string.h> /* size_t */
#include <inttypes.h> /* uint32_t */
#include "config.h"

enum pcm_dma_status
{
#ifdef HAVE_SPDIF_REC
    PCM_DMAST_ERR_SPDIF = -2,
#endif
    PCM_DMAST_ERR_DMA   = -1,
    PCM_DMAST_OK        =  0,
    PCM_DMAST_STARTED   =  1,
};

/** RAW PCM routines used with playback and recording **/

/* Typedef for registered data callback */
typedef void (*pcm_play_callback_type)(const void **start, size_t *size);

/* Typedef for registered status callback */
typedef enum pcm_dma_status (*pcm_status_callback_type)(enum pcm_dma_status status);

/* set the pcm frequency - use values in hw_sampr_list 
 * when CONFIG_SAMPR_TYPES is #defined, or-in SAMPR_TYPE_* fields with
 * frequency value. SAMPR_TYPE_PLAY is 0 and the default if none is
 * specified. */
#ifdef CONFIG_SAMPR_TYPES
#ifdef SAMPR_TYPE_REC
unsigned int pcm_sampr_type_rec_to_play(unsigned int samplerate);
#endif
#endif /* CONFIG_SAMPR_TYPES */

void pcm_set_frequency(unsigned int samplerate);
/* apply settings to hardware immediately */
void pcm_apply_settings(void);

/** RAW PCM playback routines **/

/* Reenterable locks for locking and unlocking the playback interrupt */
void pcm_play_lock(void);
void pcm_play_unlock(void);

void pcm_init(void) INIT_ATTR;
void pcm_postinit(void);
bool pcm_is_initialized(void);

/* This is for playing "raw" PCM data */
void pcm_play_data(pcm_play_callback_type get_more,
                   pcm_status_callback_type status_cb,
                   const void *start, size_t size);

/* Kept internally for global PCM and used by mixer's verion of peak
   calculation */
struct pcm_peaks
{
    uint32_t left;  /* Left peak value */
    uint32_t right; /* Right peak value */
    long period;    /* For tracking calling period */
    long tick;      /* Last tick called */
};

void pcm_calculate_peaks(int *left, int *right);
const void* pcm_get_peak_buffer(int* count);
size_t pcm_get_bytes_waiting(void);

void pcm_play_stop(void);
void pcm_play_pause(bool play);
bool pcm_is_paused(void);
bool pcm_is_playing(void);

#ifdef HAVE_RECORDING

/** RAW PCM recording routines **/

/* Typedef for registered data callback */
typedef void (*pcm_rec_callback_type)(void **start, size_t *size);

/* Reenterable locks for locking and unlocking the recording interrupt */
void pcm_rec_lock(void);
void pcm_rec_unlock(void);

/* Initialize pcm recording interface */
void pcm_init_recording(void);
/* Uninitialize pcm recording interface */
void pcm_close_recording(void);

/* Start recording "raw" PCM data */
void pcm_record_data(pcm_rec_callback_type more_ready,
                     pcm_status_callback_type status_cb,
                     void *start, size_t size);

/* Stop tranferring data into supplied buffer */
void pcm_stop_recording(void);

/* Is pcm currently recording? */
bool pcm_is_recording(void);

void pcm_calculate_rec_peaks(int *left, int *right);

#endif /* HAVE_RECORDING */

#endif /* PCM_PLAYBACK_H */
