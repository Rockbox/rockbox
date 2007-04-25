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
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef PCM_RECORD_H
#define PCM_RECORD_H

#define DMA_REC_ERROR_DMA       (-1)
#ifdef HAVE_SPDIF_IN
#define DMA_REC_ERROR_SPDIF     (-2)
#endif

/** Warnings **/
/* pcm (dma) buffer has overflowed */
#define PCMREC_W_PCM_BUFFER_OVF         0x00000001
/* encoder output buffer has overflowed */
#define PCMREC_W_ENC_BUFFER_OVF         0x00000002
#ifdef PCMREC_PARANOID
/* dma write position alignment incorrect */
#define PCMREC_W_DMA_WR_POS_ALIGN       0x00000004
/* pcm read position changed at some point not under control of recording */
#define PCMREC_W_PCM_RD_POS_TRASHED     0x00000008
/* dma write position changed at some point not under control of recording */
#define PCMREC_W_DMA_WR_POS_TRASHED     0x00000010
#endif /* PCMREC_PARANOID */
/** Errors **/
/* failed to load encoder */
#define PCMREC_E_LOAD_ENCODER           0x80001000
/* error originating in encoder */
#define PCMREC_E_ENCODER                0x80002000
/* filename queue has desynced from stream markers */
#define PCMREC_E_FNQ_DESYNC             0x80004000
/* I/O error has occurred */
#define PCMREC_E_IO                     0x80008000
#ifdef PCMREC_PARANOID
/* encoder has written past end of allotted space */
#define PCMREC_E_CHUNK_OVF              0x80010000
/* chunk header incorrect */
#define PCMREC_E_BAD_CHUNK              0x80020000
/* encoder read position changed outside of recording control */
#define PCMREC_E_ENC_RD_INDEX_TRASHED   0x80040000
/* encoder write position changed outside of recording control */
#define PCMREC_E_ENC_WR_INDEX_TRASHED   0x80080000
#endif /* PCMREC_PARANOID */

/**
 * RAW pcm data recording
 * These calls are nescessary only when using the raw pcm apis directly.
 */

/* Initialize pcm recording interface */
void pcm_init_recording(void);
/* Uninitialze pcm recording interface */
void pcm_close_recording(void);

/* Start recording "raw" PCM data */
void pcm_record_data(pcm_more_callback_type2 more_ready,
                     void *start, size_t size);

/* Stop tranferring data into supplied buffer */
void pcm_stop_recording(void);

/* Continue transferring data in - call during interrupt handler */
void pcm_record_more(void *start, size_t size);

void pcm_calculate_rec_peaks(int *left, int *right);

/** General functions for high level codec recording **/
/* pcm_rec_error_clear is deprecated for general use. audio_error_clear
   should be used */
void pcm_rec_error_clear(void);
/* pcm_rec_status is deprecated for general use. audio_status merges the
   results for consistency with the hardware codec version */
unsigned long pcm_rec_status(void);
unsigned long pcm_rec_get_warnings(void);
void pcm_rec_init(void);
int  pcm_rec_current_bitrate(void);
int  pcm_rec_encoder_afmt(void); /* AFMT_* value, AFMT_UNKNOWN if none */
int  pcm_rec_rec_format(void);   /* Format index or -1 otherwise */
unsigned long pcm_rec_sample_rate(void);
int  pcm_get_num_unprocessed(void);

/* audio.h contains audio_* recording functions */


/** The following are for internal use between pcm_record.c and target-
    specific portion **/
/* the registered callback function for when more data is available */
extern volatile pcm_more_callback_type2 pcm_callback_more_ready;
/* DMA transfer in is currently active */
extern volatile bool                    pcm_recording;

/* APIs implemented in the target-specific portion */
extern void pcm_rec_dma_start(void *addr, size_t size);
extern void pcm_rec_dma_stop(void);

#endif /* PCM_RECORD_H */
