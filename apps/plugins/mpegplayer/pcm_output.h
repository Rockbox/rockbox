/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * PCM output buffer declarations
 *
 * Copyright (c) 2007 Michael Sevakis
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef PCM_OUTPUT_H
#define PCM_OUTPUT_H

struct pcm_frame_header   /* Header added to pcm data every time a decoded
                             audio frame is sent out */
{
    uint32_t size;        /* size of this frame - including header */
    uint32_t time;        /* timestamp for this frame in audio ticks */
    unsigned char data[]; /* open array of audio data */
} ALIGNED_ATTR(4);

extern int pcm_skipped, pcm_underruns;

bool pcm_output_init(void);
void pcm_output_exit(void);
void pcm_output_flush(void);
void pcm_output_set_clock(uint32_t time);
uint32_t pcm_output_get_clock(void);
uint32_t pcm_output_get_ticks(uint32_t *start);
void pcm_output_play_pause(bool play);
void pcm_output_stop(void);
void pcm_output_drain(void);
struct pcm_frame_header * pcm_output_get_buffer(void);
void pcm_output_add_data(void);
ssize_t pcm_output_used(void);
ssize_t pcm_output_free(void);

#endif /* PCM_OUTPUT_H */
