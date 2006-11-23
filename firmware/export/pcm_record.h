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

#define DMA_REC_ERROR_DMA       ((size_t)-1)
#ifdef HAVE_SPDIF_IN
#define DMA_REC_ERROR_SPDIF     ((size_t)-2)
#endif

/**
 * RAW pcm data recording
 * These calls are nescessary only when using the raw pcm apis directly.
 */

/* Initialize pcm recording interface */
void pcm_init_recording(void);
/* Uninitialze pcm recording interface */
void pcm_close_recording(void);

/* Start recording "raw" PCM data */
void pcm_record_data(pcm_more_callback_type more_ready,
                     unsigned char *start, size_t size);

/* Stop tranferring data into supplied buffer */
void pcm_stop_recording(void);

void pcm_calculate_rec_peaks(int *left, int *right);

/** General functions for high level codec recording **/
/* pcm_rec_error_clear is deprecated for general use. audio_error_clear
   should be used */
/* void pcm_rec_error_clear(void); */
/* pcm_rec_status is deprecated for general use. audio_status merges the
   results for consistency with the hardware codec version */
/* unsigned long pcm_rec_status(void); */
void pcm_rec_init(void);
int  pcm_rec_current_bitrate(void);
int  pcm_rec_encoder_afmt(void); /* AFMT_* value, AFMT_UNKNOWN if none */
int  pcm_rec_rec_format(void);   /* Format index or -1 otherwise */
unsigned long pcm_rec_sample_rate(void);
int  pcm_get_num_unprocessed(void);

/* audio.h contains audio_* recording functions */

#endif /* PCM_RECORD_H */
