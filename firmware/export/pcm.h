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

#include <sys/types.h>

#define DMA_REC_ERROR_DMA       (-1)
#ifdef HAVE_SPDIF_REC
#define DMA_REC_ERROR_SPDIF     (-2)
#endif

/** Warnings **/
/* pcm (dma) buffer has overflowed */
#define PCMREC_W_PCM_BUFFER_OVF         0x00000001
/* encoder output buffer has overflowed */
#define PCMREC_W_ENC_BUFFER_OVF         0x00000002
/** Errors **/
/* failed to load encoder */
#define PCMREC_E_LOAD_ENCODER           0x80001000
/* error originating in encoder */
#define PCMREC_E_ENCODER                0x80002000
/* filename queue has desynced from stream markers */
#define PCMREC_E_FNQ_DESYNC             0x80004000
/* I/O error has occurred */
#define PCMREC_E_IO                     0x80008000
#ifdef DEBUG
/* encoder has written past end of allotted space */
#define PCMREC_E_CHUNK_OVF              0x80010000
#endif /* DEBUG */

/** RAW PCM routines used with playback and recording **/

/* Typedef for registered callback */
typedef void (*pcm_more_callback_type)(unsigned char **start,
                                       size_t *size);
typedef int (*pcm_more_callback_type2)(int status);

/* set the pcm frequency - use values in hw_sampr_list
 * use -1 for the default frequency
 */
void pcm_set_frequency(unsigned int samplerate);
/* apply settings to hardware immediately */
void pcm_apply_settings(void);

/** RAW PCM playback routines **/

/* Reenterable locks for locking and unlocking the playback interrupt */
void pcm_play_lock(void);
void pcm_play_unlock(void);

void pcm_init(void);
void pcm_postinit(void);

/* This is for playing "raw" PCM data */
void pcm_play_data(pcm_more_callback_type get_more,
                   unsigned char* start, size_t size);

void pcm_calculate_peaks(int *left, int *right);
size_t pcm_get_bytes_waiting(void);

void pcm_play_stop(void);
void pcm_mute(bool mute);
void pcm_play_pause(bool play);
bool pcm_is_paused(void);
bool pcm_is_playing(void);

/** The following are for internal use between pcm.c and target-
    specific portion **/

extern unsigned long pcm_curr_sampr;
extern unsigned long pcm_sampr;
extern int pcm_fsel;

/* the registered callback function to ask for more mp3 data */
extern volatile pcm_more_callback_type pcm_callback_for_more;
extern volatile bool                   pcm_playing;
extern volatile bool                   pcm_paused;

void pcm_play_dma_lock(void);
void pcm_play_dma_unlock(void);
void pcm_play_dma_init(void);
void pcm_play_dma_start(const void *addr, size_t size);
void pcm_play_dma_stop(void);
void pcm_play_dma_pause(bool pause);
void pcm_play_dma_stopped_callback(void);
const void * pcm_play_dma_get_peak_buffer(int *count);

void pcm_dma_apply_settings(void);

#ifdef HAVE_RECORDING

/** RAW PCM recording routines **/

/* Reenterable locks for locking and unlocking the recording interrupt */
void pcm_rec_lock(void);
void pcm_rec_unlock(void);

/* Initialize pcm recording interface */
void pcm_init_recording(void);
/* Uninitialze pcm recording interface */
void pcm_close_recording(void);

/* Start recording "raw" PCM data */
void pcm_record_data(pcm_more_callback_type2 more_ready,
                     void *start, size_t size);

/* Stop tranferring data into supplied buffer */
void pcm_stop_recording(void);

/* Is pcm currently recording? */
bool pcm_is_recording(void);

/* Continue transferring data in - call during interrupt handler */
void pcm_record_more(void *start, size_t size);

void pcm_calculate_rec_peaks(int *left, int *right);

/** The following are for internal use between pcm.c and target-
    specific portion **/
extern volatile const void *pcm_rec_peak_addr;
/* the registered callback function for when more data is available */
extern volatile pcm_more_callback_type2 pcm_callback_more_ready;
/* DMA transfer in is currently active */
extern volatile bool                    pcm_recording;

/* APIs implemented in the target-specific portion */
void pcm_rec_dma_init(void);
void pcm_rec_dma_close(void);
void pcm_rec_dma_start(void *addr, size_t size);
void pcm_rec_dma_stop(void);
void pcm_rec_dma_stopped_callback(void);
const void * pcm_rec_dma_get_peak_buffer(int *count);

#endif /* HAVE_RECORDING */

#endif /* PCM_PLAYBACK_H */
