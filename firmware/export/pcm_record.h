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

void          enc_set_parameters(int chunk_size, int num_chunks,
                    int samp_per_chunk, char *head_ptr, int head_size,
                    int enc_id);
void          enc_get_inputs(int *buffer_size, int *channels, int *quality);
unsigned int* enc_alloc_chunk(void);
void          enc_free_chunk(void);
int           enc_wavbuf_near_empty(void);
char*         enc_get_wav_data(int size);
extern void   (*enc_set_header_callback)(void *head_buffer, int head_size,
                                int num_pcm_samples, bool is_file_header);

unsigned long pcm_rec_status(void);
void pcm_rec_init(void);
void pcm_rec_mux(int source);
int  pcm_rec_current_bitrate(void);
int  pcm_get_num_unprocessed(void);
void pcm_rec_get_peaks(int *left, int *right);

/* audio.h contains audio recording functions */

#endif
